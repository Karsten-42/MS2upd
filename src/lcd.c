/*
* ----------------------------------------------------------------------------
* Low level driver for interfacing a HD44780U-based text LCD display
*
* Based on Peter Fleury <pfleury@gmx.ch> lcd lib 1.15.2.2 
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd.h"
#include "utils.h"

// constants/macros 
#define lcd_e_delay()   _delay_us(LCD_DELAY_ENABLE_PULSE)
#define delay(us)  _delay_us(us) 

#if LCD_LINES==1
#define LCD_FUNCTION_DEFAULT    LCD_FUNCTION_4BIT_1LINE 
#else
#define LCD_FUNCTION_DEFAULT    LCD_FUNCTION_4BIT_2LINES 
#endif


// -----------------------------------------------------------------------------
// Description: Toggle enable bit for given time
//
// Called by: divers
//
// Return: void
// ----------------------------------------------------------------------------
static void
toggle_e(void)	{

	SET(LCD_E);
	lcd_e_delay();
	RESET(LCD_E);
}

// -----------------------------------------------------------------------------
// Description: write a byte to LCD controller
//
// Called by: divers
//
// Input:	data byte tor write to LCD
//          rs     1: write data    
//                 0: write instruction
//
// Return: void
// ----------------------------------------------------------------------------
static
void lcd_write(uint8_t data,uint8_t rs)	{

	if (rs)         // write data (RS=1, RW=0) 
       SET(LCD_RS);
	else
		RESET(LCD_RS);

	RESET(LCD_RW);	// RW=0  write mode

	//configure data pins as output */
	SET_OUTPUT(LCD_DATA0);
	SET_OUTPUT(LCD_DATA1);
	SET_OUTPUT(LCD_DATA2);
	SET_OUTPUT(LCD_DATA3);
        
	//output high nibble first
	RESET(LCD_DATA3);
	RESET(LCD_DATA2);
	RESET(LCD_DATA1);
	RESET(LCD_DATA0);
	if(data & 0x80) SET(LCD_DATA3);
	if(data & 0x40) SET(LCD_DATA2);
	if(data & 0x20) SET(LCD_DATA1);
	if(data & 0x10) SET(LCD_DATA0);
	toggle_e();
        
	//output low nibble
	RESET(LCD_DATA3);
	RESET(LCD_DATA2);
	RESET(LCD_DATA1);
	RESET(LCD_DATA0);
	if(data & 0x08) SET(LCD_DATA3);
	if(data & 0x04) SET(LCD_DATA2);
	if(data & 0x02) SET(LCD_DATA1);
	if(data & 0x01) SET(LCD_DATA0);
	toggle_e();        
        
	//all data pins high (inactive)
	RESET(LCD_DATA3);
	RESET(LCD_DATA2);
	RESET(LCD_DATA1);
	RESET(LCD_DATA0);
}


// -----------------------------------------------------------------------------
// Description: read a byte from LCD controller
//
// Called by: divers
//
// Input:	rs     1: read data    
//                 0: read busy flag / address counter
//
// Return: void
// ----------------------------------------------------------------------------
static uint8_t
lcd_read(uint8_t rs)	{

	uint8_t data;
    
	if (rs)
		SET(LCD_RS);	// RS=1: read data
	else
		RESET(LCD_RS);	// RS=0: read busy flag

	SET(LCD_RW);		// RW=1  read mode
    
	//configure data pins as input
	SET_INPUT(LCD_DATA0); 
	SET_INPUT(LCD_DATA1); 
	SET_INPUT(LCD_DATA2); 
	SET_INPUT(LCD_DATA3); 
	 
	//read high nibble first
	SET(LCD_E);
	lcd_e_delay();        
	data = 0;
	if(IS_SET(LCD_DATA0)) data |= 0x10;
	if(IS_SET(LCD_DATA1)) data |= 0x20;
	if(IS_SET(LCD_DATA2)) data |= 0x40;
	if(IS_SET(LCD_DATA3)) data |= 0x80;
	RESET(LCD_E);

	lcd_e_delay();	// Enable 500ns low 

	//read low nibble
	SET(LCD_E);
	lcd_e_delay();
	if(IS_SET(LCD_DATA0)) data |= 0x01;
	if(IS_SET(LCD_DATA1)) data |= 0x02;
	if(IS_SET(LCD_DATA2)) data |= 0x04;
	if(IS_SET(LCD_DATA3)) data |= 0x08;
	RESET(LCD_E);

	return data;
}


// -----------------------------------------------------------------------------
// Description: Loops while LCD is busy, return address counter
//
// Called by: divers
//
// Input:  void
//
// Return: address counter
// ----------------------------------------------------------------------------
static uint8_t
lcd_waitbusy(void)	{

	register uint8_t c;

	// wait until busy flag is cleared 
	while ( (c=lcd_read(0)) & (1<<LCD_BUSY)) {}

	// the address counter is updated 4us after the busy flag is cleared
	delay(LCD_DELAY_BUSY_FLAG);

	// now read the address counter
	return (lcd_read(0));  // return address counter
}


// -----------------------------------------------------------------------------
// Description: Move cursor to the start of next line or to the first line
// 				 if the cursor is already on the last line.
//
// Called by: divers
//
// Input:  pos	
//
// Return: void
// ----------------------------------------------------------------------------
static inline void
lcd_newline(uint8_t pos)	{

	register uint8_t addressCounter;

#if LCD_LINES==1
	addressCounter = 0;
#endif
#if LCD_LINES==2
	if ( pos < (LCD_START_LINE2) )
	addressCounter = LCD_START_LINE2;
	else
	addressCounter = LCD_START_LINE1;
#endif

#if LCD_LINES==4
	if ( pos < LCD_START_LINE3 )
		addressCounter = LCD_START_LINE2;
	else if ( (pos >= LCD_START_LINE2) && (pos < LCD_START_LINE4) )
		addressCounter = LCD_START_LINE3;
	else if ( (pos >= LCD_START_LINE3) && (pos < LCD_START_LINE2) )
		addressCounter = LCD_START_LINE4;
	else 
		addressCounter = LCD_START_LINE1;
#endif

	lcd_command((1<<LCD_DDRAM)+addressCounter);
}


// -----------------------------------------------------------------------------
// ---------*** PUBLIC Functions ***-------------
// -----------------------------------------------------------------------------

/*************************************************************************
Send LCD controller instruction command
Input:   instruction to send to LCD controller, see HD44780 data sheet
Returns: none
*************************************************************************/
void
lcd_command(uint8_t cmd)	{

	lcd_waitbusy();
	lcd_write(cmd,0);
}


/*************************************************************************
Send data byte to LCD controller 
Input:   data to send to LCD controller, see HD44780 data sheet
Returns: none
*************************************************************************/
void
lcd_data(uint8_t data)	{

	lcd_waitbusy();
	lcd_write(data,1);
}



/*************************************************************************
Set cursor to specified position
Input:    x  horizontal position  (0: left most position)
          y  vertical position    (0: first line)
Returns:  none
*************************************************************************/
void
lcd_gotoxy(uint8_t x, uint8_t y)	{

#if LCD_LINES==1
    lcd_command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
#endif
#if LCD_LINES==2
    if ( y==0 ) 
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
    else
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE2+x);
#endif
#if LCD_LINES==4
    if ( y==0 )
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
    else if ( y==1)
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE2+x);
    else if ( y==2)
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE3+x);
    else // y==3 
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE4+x);
#endif

}


/*************************************************************************
Clear display and set cursor to home position
*************************************************************************/
void
lcd_clrscr(void)	{

	lcd_command(1<<LCD_CLR);
}


/*************************************************************************
Set cursor to home position
*************************************************************************/
void
lcd_home(void)	{

	lcd_command(1<<LCD_HOME);
}


/*************************************************************************
Display character at current cursor position 
Input:    character to be displayed                                       
Returns:  none
*************************************************************************/
void
lcd_putc(char c)	{

	uint8_t pos;


	pos = lcd_waitbusy();   // read busy-flag and address counter
	if (c=='\n')	{

	  lcd_newline(pos);

	} else	{

	#if LCD_WRAP_LINES==1
		#if LCD_LINES==1
		if ( pos == LCD_START_LINE1+LCD_DISP_LENGTH ) {
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE1,0);
		}
		#elif LCD_LINES==2
		if ( pos == LCD_START_LINE1+LCD_DISP_LENGTH ) {
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE2,0);    
		}else if ( pos == LCD_START_LINE2+LCD_DISP_LENGTH ){
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE1,0);
		}
		#elif LCD_LINES==4
		if ( pos == LCD_START_LINE1+LCD_DISP_LENGTH ) {
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE2,0);    
		}else if ( pos == LCD_START_LINE2+LCD_DISP_LENGTH ) {
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE3,0);
		}else if ( pos == LCD_START_LINE3+LCD_DISP_LENGTH ) {
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE4,0);
		}else if ( pos == LCD_START_LINE4+LCD_DISP_LENGTH ) {
			lcd_write((1<<LCD_DDRAM)+LCD_START_LINE1,0);
		}
		#endif

		lcd_waitbusy();
	#endif
		lcd_write(c, 1);
	}

}


/*************************************************************************
Display string without auto linefeed 
Input:    string to be displayed
Returns:  none
print string on lcd (no auto linefeed)
*************************************************************************/
void
lcd_puts(const char *s)	{

	register char c;

	while ( (c = *s++) ) {
		lcd_putc(c);
	} 
}


/*************************************************************************
Display string from program memory without auto linefeed 
Input:     string from program memory be be displayed                                        
Returns:   none
print string from program memory on lcd (no auto linefeed)
*************************************************************************/
void
lcd_puts_p(const char *progmem_s)	{

	register char c;

	while ( (c = pgm_read_byte(progmem_s++)) ) {
		lcd_putc(c);
	}
}


/*************************************************************************
Initialize display and select type of cursor 
Input:    dispAttr LCD_DISP_OFF            display off
                   LCD_DISP_ON             display on, cursor off
                   LCD_DISP_ON_CURSOR      display on, cursor on
                   LCD_DISP_CURSOR_BLINK   display on, cursor on flashing
Returns:  none
*************************************************************************/
void
lcd_init(uint8_t dispAttr)	{


   // Initialize LCD to 4 bit I/O mode
	SET_OUTPUT(LCD_RS);
	SET_OUTPUT(LCD_RW);
	SET_OUTPUT(LCD_E);
	SET_OUTPUT(LCD_DATA0);
	SET_OUTPUT(LCD_DATA1);
	SET_OUTPUT(LCD_DATA2);
	SET_OUTPUT(LCD_DATA3);

	delay(LCD_DELAY_BOOTUP);	// wait 16ms or more after power-on
    
	// Initial write to lcd is 8bit 
	SET(LCD_DATA1);				// LCD_FUNCTION>>4;
	SET(LCD_DATA0);				// LCD_FUNCTION_8BIT>>4;
	toggle_e();
	delay(LCD_DELAY_INIT);		// delay, busy flag can't be checked here
   
	// repeat last command/ 
	toggle_e();      
	delay(LCD_DELAY_INIT_REP);	// delay, busy flag can't be checked here

	// repeat last command a third time/
	toggle_e();      
	delay(LCD_DELAY_INIT_REP);	// delay, busy flag can't be checked here 

	// now configure for 4bit mode 
	RESET(LCD_DATA0);
	toggle_e();
	delay(LCD_DELAY_INIT_4BIT);	// some displays need this additional delay

	// from now the LCD only accepts 4 bit I/O, we can use lcd_command()
	lcd_command(LCD_FUNCTION_DEFAULT);	// function set: display lines
	lcd_command(LCD_DISP_OFF);				// display off               
	lcd_clrscr();								// display clear            
	lcd_command(LCD_MODE_DEFAULT);		// set entry mode          
	lcd_command(dispAttr);					// display/cursor control 
}
