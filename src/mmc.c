/*
* ----------------------------------------------------------------------------
* Low level driver for mmc cards need for ELM FatFs
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#include "mmc.h"

static void			init_timer(void);
static uint8_t 	mmc_enable(void);
static void 		mmc_disable(void); 
static uint8_t 	mmc_wait_ready (void);
static uint8_t		mmc_send_cmd (	uint8_t cmd, uint32_t arg);
static int			mmc_rx_datablock( uint8_t *buff, uint16_t btr);
#if (TXB0104_OE == TRUE)
static void			mmc_powerOn(void);
static void			mmc_powerOff(void);
#endif


static BYTE CardType; //Cardtype (b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing)
static volatile DSTATUS Stat = STA_NOINIT;	// Disk status 
volatile uint8_t TimingDelay;

#include "uart.h"
#include <avr/pgmspace.h>

// -----------------------------------------------------------------------------
// ---------*** PUBLIC Functions as low level interface for ELM ***-------------
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Description: Initialize  SD Card
//
// Details: Elm-ChaN low level
// 
// Called by: FatFS API in ff.c
//
// Return: Status byte
// ----------------------------------------------------------------------------
DSTATUS
disk_initialize(BYTE pdrv)	{

	uint8_t cmd, ty, ocr[4];
	uint16_t n, j;

	init_timer(); // Need 10ms increment of TimingDelay var

	#if (TXB0104_OE == TRUE)
		mmc_powerOn();
	#endif

	mmc_disable();

	for (n = 100; n; n--) spi_read_byte();	// 80+ dummy clocks

	ty = 0;
	j=100;
	do {

		if (mmc_send_cmd(CMD0, 0) == 1) {  	// Enter Idle state

			j=0;
			TimingDelay = 100;        	// Initialization timeout of 1000 msec

			if (mmc_send_cmd(CMD8, 0x1AA) == 1) {	// SDv2?

				for (n = 0; n < 4; n++){
					ocr[n] = spi_read_byte();  // Get trailing retrn value of R7 resp
				}

				if (ocr[2] == 0x01 && ocr[3] == 0xAA) { // The card can work at vdd range of 2.7-3.6V
					while (TimingDelay) { // Wait for leaving idle state (ACMD41 with HCS bit)
						mmc_send_cmd(CMD55, 0);
						if(!mmc_send_cmd(ACMD41, 1UL << 30))
							break;
					}

					while(TimingDelay) {

						if (mmc_send_cmd(CMD58, 0) == 0x00) { // Check CCS bit in the OCR
							for (n = 0; n < 4; n++){
								ocr[n] = spi_read_byte();
							}

							ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  // SDv2
							break;
						}
					}
				}
			} else {        									// SDv1 or MMCv3

				if (mmc_send_cmd(ACMD41, 0) <= 1)   {
					ty = CT_SD1;
					cmd = ACMD41;  								// SDv1
				} else {
					ty = CT_MMC;
					cmd = CMD1;    								// MMCv3
				}

				while (TimingDelay && mmc_send_cmd(cmd, 0)); // Wait for leaving idle state
			}

			if(ty != (CT_SD2 | CT_BLOCK)) {
				while(TimingDelay && (mmc_send_cmd(CMD16, 512) != 0));
			}

			if(!TimingDelay) ty = 0;

		} else { j--; }

	}while(j>0);

	CardType = ty;
	mmc_disable();

	if(ty)	{
		Stat &= ~STA_NOINIT;
		#if (SPI_MAX_SPEED==TRUE)
			spi_maxSpeed();
		#endif
	} else {

		#if (TXB0104_OE == TRUE)
			mmc_powerOff();
		#endif
	}

	return Stat;
}

// -----------------------------------------------------------------------------
// Description: Query disk status
//
// Details: Elm-ChaN low level
// 
// Called by: FatFS API in ff.c
//
// Return: Status byte
// ----------------------------------------------------------------------------
DSTATUS
disk_status(BYTE pdrv)	{

	return Stat;
}


// -----------------------------------------------------------------------------
// Description: Read n bytes from SD Card
//
// Details: Elm-ChaN low level
// 
// Called by: FatFS API in ff.c
//
// Return: Status byte
// ----------------------------------------------------------------------------
DRESULT
disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)	{

	BYTE cmd;

	if(pdrv || !count)
		return RES_PARERR;

	if(Stat & STA_NOINIT)
		return RES_NOTRDY;

	if (!(CardType & CT_BLOCK)) sector *= 512;// Convert to byte address if needed 

	cmd = count > 1 ? CMD18 : CMD17;//  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK 
	if (mmc_send_cmd(cmd, sector) == 0) {
		do {
			if (!mmc_rx_datablock(buff, 512)) break;
			buff += 512;
		} while (--count);
		if (cmd == CMD18) mmc_send_cmd(CMD12, 0);	// STOP_TRANSMISSION 
	}

	mmc_disable();

	return count ? RES_ERROR : RES_OK;

}



// -----------------------------------------------------------------------------
// ---------*** PRIVATE Functions for low level interface for ELM ***-----------
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Description: Timer ISR
//
// Details: Must be called every 10ms. Need for timeout detection at SD-Card
// 
// Called by: Timer 2 COMPA interrupt
//
// Return: void
// ----------------------------------------------------------------------------
ISR(TIMER2_COMPA_vect)	{

	TimingDelay = (TimingDelay == 0) ? 0 : TimingDelay - 1 ;
}


// -----------------------------------------------------------------------------
// Description: Initialize timeout timer
//
// Details: Invoce every 10ms a overflow interrupt
// 
// Called by: disk_initialize(BYTE pdrv)
//
// Return: void
// ----------------------------------------------------------------------------
static void
init_timer(void)	{

	TimingDelay = 0;

	TCCR2A = (1 << WGM21); // CTC mode
	TCCR2B = (1 << CS22 ) | (1 << CS21) | (1 << CS20); // clk / 1024
	OCR2A = 156; // ISR every 10ms 
	TIMSK2 = (1 << OCIE2A);

}

// -----------------------------------------------------------------------------
// Description: Low level SPI command to SD Card
//
// Called by: FatFS API This low level implementation
//
// Return: Status byte
// ----------------------------------------------------------------------------
static uint8_t
mmc_send_cmd(uint8_t cmd,	uint32_t arg)	{
	
	uint8_t n, res;
	// Select the card and wait for ready 
	mmc_disable();
	if ( FALSE == mmc_enable() ){
		return 0xFF;
	}
	// Send command packet 
	spi_write_byte(0x40 | cmd);						// Start + Command index 
	spi_write_byte( (uint8_t)(arg >> 24) );	// Argument[31..24]
	spi_write_byte( (uint8_t)(arg >> 16) );	// Argument[23..16]
	spi_write_byte( (uint8_t)(arg >> 8) );	// Argument[15..8]
	spi_write_byte( (uint8_t)arg );			// Argument[7..0]
	n = 0x01;										// Dummy CRC + Stop 
	if (cmd == CMD0) n = 0x95;						// Valid CRC for CMD0(0) 
	if (cmd == CMD8) n = 0x87;						// Valid CRC for CMD8(0x1AA) 
	spi_write_byte(n);

	// Receive command response 
	if (cmd == CMD12) spi_read_byte();	// Skip a stuff byte when stop reading 
	n = 10;					// Wait for a valid response in timeout of 10 attempts 
	do
		res = spi_read_byte();
	while ( (res & 0x80) && --n );

	return res;										// Return with the response value 
}

// -----------------------------------------------------------------------------
// Description: Low level SPI read bytes from SD Card
//
// Called by: FatFS API This low level implementation
//
// Return: Status byte
// ----------------------------------------------------------------------------
static int
mmc_rx_datablock( uint8_t *buff, uint16_t btr)	{

	BYTE token;

	TimingDelay = 20;       	// Initialization timeout of 200 msec

	do {							// Wait for data packet in timeout of 200ms
		token = spi_read_byte();
	} while ((token == 0xFF) && TimingDelay);

	if (token != 0xFE) return 0;	// If not valid data token, retutn with error

	spi_read_block(buff, btr);		// Receive the data block into buffer

	spi_write_byte(0xFF);			// Discard CRC 
	spi_write_byte(0xFF);					

	return 1;						// Return with success
}


// -----------------------------------------------------------------------------
// Description: Low level SPI enable and wait for SD Card
//
// Called by: FatFS API This low level implementation
//
// Return: TRUE on success
// ----------------------------------------------------------------------------
static uint8_t
mmc_enable()	{
      
   MMC_CS_LOW;
   if( !mmc_wait_ready() ){
   	  mmc_disable();
	  return FALSE;
   }

   return TRUE;
}

// -----------------------------------------------------------------------------
// Description: Low level SPI disable 
//
// Called by: FatFS API This low level implementation
//
// Return: void
// ----------------------------------------------------------------------------
static void
mmc_disable()	{

   MMC_CS_HIGH;   
   spi_read_byte();
}


// -----------------------------------------------------------------------------
// Description: Low level SPI wait max 500ms for SD-Card  response
//
// Called by: FatFS API This low level implementation
//
// Return: TRUE on success
// ----------------------------------------------------------------------------
static uint8_t
mmc_wait_ready (void){

	TimingDelay = 50;

	do{
		if(	 spi_read_byte() == 0xFF ) return TRUE;
	}while ( TimingDelay );

	return FALSE;
}


#if (TXB0104_OE == TRUE)
// -----------------------------------------------------------------------------
// Description: Low level switch SD-card ON
//
// Details: Implementation OE set to HIGH for TXB01014 Level-Shifter
//
// Called by: FatFS API This low level implementation
//
// Return: void
// ----------------------------------------------------------------------------
static void
mmc_powerOn(void)	{

}

// -----------------------------------------------------------------------------
// Description: Low level switch SD-card OFF
//
// Details: Implementation OE set to LOW for TXB01014 Level-Shifter
//
// Called by: FatFS API This low level implementation
//
// Return: void
// ----------------------------------------------------------------------------
static void
mmc_powerOff(void)	{

}
#endif
