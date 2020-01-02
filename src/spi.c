/*
* ----------------------------------------------------------------------------
* Simple SPI lib
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#include "spi.h"

void
spi_init(void)	{

	// CS of all devices
	SET_OUTPUT(MCP_CS);
	SET_OUTPUT(SD_CS);

	RESET(SPI_SCK);
	RESET(SPI_MOSI);
	RESET(SPI_MISO);
	
	SET_OUTPUT(SPI_SCK);
	SET_OUTPUT(SPI_MOSI);
	SET_INPUT(SPI_MISO);
	
	// hardware spi: bus clock = idle low, spi clock / 128 , spi master mode
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPR1);

	SET(MCP_CS);
	SET(SD_CS);
}


#if SPI_MAX_SPEED
// *****************************************************************************
void
spi_maxSpeed() {
	
	//Set SPI speed to clock / 2 
	SPCR &= ~((1<<SPR0) | (1<<SPR1));
	SPSR |= (1<<SPI2X);
}
#endif

// *****************************************************************************
void
spi_write_byte(uint8_t byte) {
	
	SPDR = byte;
	loop_until_bit_is_set(SPSR,SPIF);
}


// *****************************************************************************
uint8_t
spi_read_byte(void) {
	
  SPDR = 0xff;
  loop_until_bit_is_set(SPSR,SPIF);
  return (SPDR);
}

// *****************************************************************************
void
spi_read_block(uint8_t *p, uint16_t cnt)	{

	do {
			SPDR = 0xFF;
			loop_until_bit_is_set(SPSR, SPIF);
			*p++ = SPDR;
			SPDR = 0xFF;
			loop_until_bit_is_set(SPSR, SPIF);
			*p++ = SPDR;
		} while (cnt -= 2);

}
