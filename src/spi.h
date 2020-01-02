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

#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdio.h>
#include "utils.h"

//prototypes
void spi_init(void);
void spi_maxSpeed(void);
void spi_write_byte(uint8_t byte);
uint8_t spi_read_byte(void);
void spi_read_block(uint8_t *p, uint16_t cnt);

// ----------------------------------------------------------------------------
extern __attribute__ ((gnu_inline)) inline void spi_start(uint8_t data) {
	SPDR = data;
}

// ----------------------------------------------------------------------------
extern __attribute__ ((gnu_inline)) inline uint8_t spi_wait(void) {
	// warten bis der vorherige Werte geschrieben wurde
	while(!(SPSR & (1<<SPIF)))
		;
	
	return SPDR;
}

#ifndef TRUE
	#define TRUE 	0x01
	#define FALSE 	0x00
#endif

// enable to run SPI in maximal speed
//#define SPI_MAX_SPEED	TRUE

// ATMEGA328 HW SPI 
#define	SPI_MOSI	B,3
#define	SPI_MISO	B,4
#define	SPI_SCK	B,5

// Chip Select port pin to select SPI devices
// Ensure, one of then is the SPI_SS pin to enable SPI Master mode
#define	MCP_CS B,2 	//CAN Controller (SPI_SS)
#define	SD_CS B,1 	//SD Card reader

// Short macros to set/reset chip select line
#define MCP_CS_LOW 		RESET(MCP_CS)
#define MCP_CS_HIGH		SET(MCP_CS)	

#define MMC_CS_LOW 		RESET(SD_CS)	
#define MMC_CS_HIGH		SET(SD_CS)


#endif
