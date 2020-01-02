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

#ifndef _MMC_H
#define _MMC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/io.h>	
#include "ff.h"
#include "diskio.h"
#include "spi.h"

#define SPI_MAX_SPEED FALSE	// TRUE Switch max SPI speed after init MMC_Card
#define TXB0104_OE FALSE		// If HW need to drive TXB0104 by !OE select

// Definitions for MMC/SDC command 
#define CMD0	(0)			// GO_IDLE_STATE 
#define CMD1	(1)			// SEND_OP_COND (MMC) 
#define	ACMD41	(41)		// SEND_OP_COND (SDC)
#define CMD8	(8)			// SEND_IF_COND 
#define CMD9	(9)			// SEND_CSD 
#define CMD10	(10)		// SEND_CID 
#define CMD12	(12)		// STOP_TRANSMISSION 
#define ACMD13	(0x80+13)	// SD_STATUS (SDC) 
#define CMD16	(16)		// SET_BLOCKLEN 
#define CMD17	(17)		// READ_SINGLE_BLOCK 
#define CMD18	(18)		// READ_MULTIPLE_BLOCK 
#define CMD23	(23)		// SET_BLOCK_COUNT (MMC) 
#define	ACMD23	(0x80+23)	// SET_WR_BLK_ERASE_COUNT (SDC) 
#define CMD24	(24)		// WRITE_BLOCK 
#define CMD25	(25)		// WRITE_MULTIPLE_BLOCK 
#define CMD55	(55)		// APP_CMD 
#define CMD58	(58)		// READ_OCR 

// Card type flags (CardType) 
#define CT_MMC		0x01			// MMC ver 3 
#define CT_SD1		0x02			// SD ver 1 
#define CT_SD2		0x04			// SD ver 2 
#define CT_SDC		(CT_SD1|CT_SD2)	// SD 
#define CT_BLOCK	0x08			// Block addressing 

#ifdef __cplusplus
}
#endif

#endif
