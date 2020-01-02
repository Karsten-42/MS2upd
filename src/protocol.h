/*
* ----------------------------------------------------------------------------
* Some Märklin CAN protocoll command collections
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef MCS2PROTOCOL_H
#define MCS2PROTOCOL_H

#include "mcs2-const.h"

// Protocoll Parts
#define MS2_BIN_MAGIC	0x4 //Magic last block number on ms2 binary transfer
#define GFP_BIN_MAGIC	0x2 //Magic last block number on 60113 binary transfer

// unique UID of MS2-Updater
#define UID0 0x4d
#define UID1 0x54
#define UID2 0x5A
#define UID3 0x6B

#define SVERS0 0x3		// define own sw version
#define SVERS1 0xC
		
// -----------------------------------------------------------------------------
// Calculation HASH:
// 16Bit High UID XOR 16Bit LOW UID
// 0x4347 XOR 0x5A6B = 0x192C ( Data from defines UID0 - UID3 )
// Initial Hash for command 0x1B will be 0x192c
// For other commands swap HByte with LByte and add b110 at Bit 9,8,7
//    0x2C |  0x19
// 00101100|00011001
// 00101111|00011001 = 0x2F19
#define UPD_HASH	0x3F17

// CRC  Polynom 
#define POLY 0x1021

typedef struct {

	uint8_t  type;
	uint16_t hash;
	uint16_t sversion;
	uint32_t uid;

} device_t;


// ----------------------------------------------------------------------------
int snd_ping(void);

// ----------------------------------------------------------------------------
int snd_cfCRC(uint16_t len);

// ----------------------------------------------------------------------------
int snd_cfStream(char *bytes, uint16_t len);

// ----------------------------------------------------------------------------
int snd_ACK(void);

// ----------------------------------------------------------------------------
int snd_sysReset(void);

// ----------------------------------------------------------------------------
int snd_bootInit(void);

// ----------------------------------------------------------------------------
int snd_bootStart(void);

// ----------------------------------------------------------------------------
int snd_binBlock(uint8_t num);

// ----------------------------------------------------------------------------
int snd_binStream(char *bytes, uint16_t len, uint8_t mode);

// ----------------------------------------------------------------------------
int snd_binCRC(void);

// ----------------------------------------------------------------------------
int resp_ping(uint16_t hash);

// ----------------------------------------------------------------------------
int resp_cfg_request(uint8_t cfgName[8]);

// ----------------------------------------------------------------------------
void create_CRC(char *data, uint16_t len, uint8_t mode);

#endif
