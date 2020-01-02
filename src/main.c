/*
* ----------------------------------------------------------------------------
* Application to load software to Märklin MS2 and Gleisbox
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
*
* Debouncing in ISR from Peter Dannecker
*
* ----------------------------------------------------------------------------
*/

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>

#include "main.h"

// static function prototypes
static void init_HW(void);
static void connect(void);
static void disconnect(void);

// Debounced button
volatile char key_state;
volatile char key_press;
static char get_key_press( char key_mask );

// CAN RX-Buffer
static can_t rx_buffer[BUF_SIZE];
volatile int posRead = 0;
volatile int posWrite = 0;
volatile bool lastOpWasWrite = false;

//Global vars
device_t device;
volatile uint16_t count = 0;


// -----------------------------------------------------------------------------
// Description: Handling generated interrupt by MCP2515. 
// 
// Details: Retrieve the CAN message and store it to the CAN buffer
//
// Called by: ISR 
//
// Return: void
// -----------------------------------------------------------------------------
ISR (INT0_vect)	{


	if (posWrite == posRead && lastOpWasWrite) {
		// Buffer Full!!!
		SET(LED_ERROR);	
		return;
	}

	if (can_get_message(&rx_buffer[posWrite])) {
		posWrite = (posWrite + 1) % BUF_SIZE;
	} 

	lastOpWasWrite = true;
}


// -----------------------------------------------------------------------------
// Description: Handling time events by using bit flags
//
// Called by: ISR will be called every 4ms @ 16MhZ
//
// Return: void
// -----------------------------------------------------------------------------
ISR (TIMER0_COMPA_vect)	{

	//TODO use uint instead char
	static char ct0, ct1;
	static uint8_t ct3 = 0;
	char i;

	i = key_state ^ ~KEY_INPUT;				// key changed ?
	ct0 = ~(ct0 & i);								// reset or count ct0
	ct1 = ct0 ^ (ct1 & i);						// reset or count ct1
	i &= ct0 & ct1;								// count until roll over ?
	key_state ^= i;								// then toggle debounced state
	key_press |= key_state & i;				// 0->1: key press detect

	ct3++;

	if(ct3 == 4)	{ // increment every 16ms and reduce processing time

//DEBUG
TOGGLE(LED_MSG);

		// remove message timeout flag
		if(count == TMOCNT)
			Flags &= ~(1 << TIMEOUT);
		
		// remove MS2 calculating message flag
		if(count == DEVCCNT)
			Flags &= ~(1 << DEVCALC);
		
		// remove MS2 connect wait flag
		if(count == DEVCNT)
			Flags &= ~(1 << DEVBOOT);

		// remove check for reinsert lost SD Card
		if(count == LOSTCNT)	
			Flags &= ~(1 << LOSTCARD);

		// remove ping count
		if(count == PNGCNT)	
			Flags &= ~(1 << FIRSTPNG);

		count++;
		ct3 = 0;
	}
}


// -----------------------------------------------------------------------------
// Description: Return a buffer from CAN buffer pool
//
// Details: A returned buffer is marked to be to used again
//
// Called by: div 
//
// Return: bool
// -----------------------------------------------------------------------------
bool
read_rx_buffer(can_t *msg)	{

	cli();

	if (posWrite == posRead && !lastOpWasWrite) {
		sei();
		return false;
	}

	memcpy(msg, &rx_buffer[posRead], sizeof(can_t));
	posRead = (posRead + 1) % BUF_SIZE;
	lastOpWasWrite = false;

	sei();
	return true;
}

// -----------------------------------------------------------------------------
// Description: Reset read and write pointer to clear rx_buffer
//
// Called by: div 
//
// Return: 
// -----------------------------------------------------------------------------
void
clear_rx_buffer(void)	{

	cli();
		posWrite = 0;
		posRead = 0;
	sei();
}

// -----------------------------------------------------------------------------
// Description: This is MAIN :-)
//
// Called by: MCU 
//
// Return: int ( hopfully not ... )
// -----------------------------------------------------------------------------
int
main(void)	{

	char lcdmsg[32];
	uint8_t state = 0;
	uint16_t filever = 0;

	init_HW();

	while(1)	{			// the BIG main loop
		
		// Loop to identify device
		while(1)	{

			lcd_clrscr();
			lcd_gotoxy(0,0);
			lcd_puts("Connect Device\n");
			lcd_puts("Wait ...\n");

			connect();
			device.type = 0;
			lcd_gotoxy(0,1);
			lcd_puts("Check ...\n");

			_delay_ms(200); // need to give CAN bus of device time to response

			if(init_MS2(&device))
				init_60113(&device);
			
			lcd_clrscr();
			switch(device.type)	{

				//TODO
				// Make filename constants dynamic or #define
				case DEV_CON_MS2:
					
					filever = get_ms2ver("050-ms2.bin");
					sprintf(lcdmsg,"Found MS2\nVer:%d.%d -> %d.%d\n",
						(device.sversion >> 8), 
						(device.sversion & 0xFF),
						(filever >> 8), 
						(filever & 0xFF));

					state = 1;
					break;

				case DEV_GFP_MS2:

					filever = get_gb2ver("016-gb2.bin");
					sprintf(lcdmsg,"Found Gleisbox\nVer:%d.%d -> %d.%d\n",
						(device.sversion >> 8), 
						(device.sversion & 0xFF),
						(filever >> 8), 
						(filever & 0xFF));

					state = 1;
					break;

				default:
					lcd_clrscr();
					lcd_gotoxy(0,0);
					lcd_puts("Unknown Device\n");
					lcd_puts("Disconnet it!\n");
					_delay_ms(1000); 
					disconnect();
					_delay_ms(1000); // prevent bouncing
					break;
			}

			if(state)
				break;

		}

		state = 0;
		lcd_puts(lcdmsg);
		_delay_ms(2000);

		// Loop until START key was pressed	
		lcd_gotoxy(0,0);
		lcd_puts("Update?  START  \n");
		while(1)	{

			// Run update sequence ( terryfying... lets cross fingers ) 
			if(get_key_press(1 << KEY_START))	{

				lcd_clrscr();
				lcd_puts("Updating");
				switch(device.type)	{

					case DEV_GFP_MS2:

						lcd_puts(" GFP Box\n");
						process_60133Update();	
						_delay_ms(2000);
						state = 1;
						break;

					case DEV_CON_MS2:

						lcd_puts(" MS2\n");
						process_MS2Update(&device);	
						_delay_ms(2000);
						state = 1;
						break;

					default:

						break;		
				}
			}

			if(state)
				break;
		}

		lcd_clrscr();
		lcd_gotoxy(0,0);
		lcd_puts(" SUCCESSFULL !\n");
		lcd_puts("Do disconnect\n");
		_delay_ms(2000); // give time to startup
		disconnect();
		_delay_ms(1000); // prevent bouncing
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Description: Initialize the hardware
//
// Called by: main
//
// Return: 0 on success othervise appropiate errno
// ----------------------------------------------------------------------------
static void
init_HW(void)	{

	uint8_t vers[9];

	// init I/O
	SET_OUTPUT(LED_ERROR);
	SET_OUTPUT(LED_MSG);
	SET(LED_ERROR);
	SET(LED_MSG);

	SET_INPUT(DEV_PRESENT);
	SET_PULLUP(DEV_PRESENT);

	SET_INPUT(KEY);
	SET_PULLUP(KEY);

	// init 8Bit Timer0, CTC, clk/1024
	TCCR0A |= (1 << WGM01);
	TCCR0B |= (1 << CS02) | (1 << CS00);
	OCR0A = 63; // 4ms @ 16Mhz
	TIMSK0 |= (1 << OCIE0A);
	
	// UART init
	uart_init(UART_BAUD_SELECT(57600UL, F_CPU));

	// activate interrups
	sei();
	
	// Redirect stdout to UART->RS232, use printf() now
	stdout = &mystdout;
	PRINT("MS2 Updater\n");

	PRINT("LCD init  OK\n");
	lcd_init(LCD_DISP_ON);
	lcd_puts(" -REAKTIVATOR-\n");
	lcd_puts(" Version 0.0.1\n");
	_delay_ms(2000);
	
	// HW SPI init
	spi_init();
	PRINT("SPI init O.K.\n");
	
	// SD-card init
	while(1)	{

		lcd_clrscr();
		lcd_puts("Check SD-Card:");
		if(init_SD())	{

			PRINT("Error: SD access FAILED\n");
			lcd_puts("NG\n");
			lcd_gotoxy(0,13);
			lcd_puts("Insert SD-Card\n");
			_delay_ms(2000);
		} else {

			lcd_puts("OK\n");
			break;
		}
	}

	// be sure sd-card and FS is working
	if(sd_open_file("016-gb2.bin"))	{

		PRINT("Cant read SD-card filesystem\n");
		lcd_clrscr();
		lcd_puts("ERROR!\n");
		lcd_puts("Read SD-Card\n");
		SET(LED_ERROR);
		while(1);
	}
	sd_read_file(vers,8);
	sd_close_file();
	PRINT("TEST file %02x %02x\n",vers[6],vers[7]);

	// HW init of CAN controller
	if(!can_init())	{
		
		PRINT("Error: MCP2515 access FAILED\n");
		lcd_clrscr();
		lcd_puts("ERROR!\n");
		lcd_puts("Init CAN\n");
		SET(LED_ERROR);
		while(1);

	} else {

		// external interrupt from MCP2515
		// Trigger on low level INT0
		EIMSK |= (1<<INT0); 
		PRINT("MCP2515 access O.K.\n\n");
	}
}

// -----------------------------------------------------------------------------
// Description: check if debounced key pressed
//
// Called by: main
//
// Return: key_mask
// ----------------------------------------------------------------------------
char
get_key_press( char key_mask )	{

  cli();
  key_mask &= key_state; 
  key_mask &= key_press;                        // read key(s)
  key_press ^= key_mask;                        // clear key(s)
  sei();
  return key_mask;
}


// -----------------------------------------------------------------------------
// Description: Wait until device connetced to physical CAN connector
//
// Details: If current used by devie > 30mA,  DEV_PRESENT is set to LOW.
//
// Called by: main()
//
// Return: 
// ----------------------------------------------------------------------------
static void
connect(void)	{

	while(1)	{

		if(! (IS_SET(DEV_PRESENT)))	{
			break;
		}
	}
}

// -----------------------------------------------------------------------------
// Description: Wait until device is disconnected from physical CAN connector
//
// Details: If current used by device < 30mA,  DEV_PRESENT is set to HIGH.
//
// Called by: main()
//
// Return: 
// ----------------------------------------------------------------------------
static void
disconnect(void)	{

	while(1)	{

		if( (IS_SET(DEV_PRESENT)))	{
			break;
		}
	}
}
