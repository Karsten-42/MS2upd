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

#include <avr/io.h>
#include <stdio.h>
#include <inttypes.h>
#include "mcp2515.h"

#include "protocol.h"

extern device_t device; // defined in main.c
static uint16_t my_crc;

static void update_crc(char ch);


// -----------------------------------------------------------------------------
// Description: Send 0x1B command current Block number
//
// Details: Decrement Blocknumber on every call. END number is MAGIC! 
// 
// Called by: 
//
// Return: sended block number
// -----------------------------------------------------------------------------
int
snd_binBlock(uint8_t num)	{

	can_t msg; 
	
	//Build ID
	msg.id = 0;							
	msg.id |= device.hash;			
	msg.id |= ((unsigned long)CMD_BOOTLD_CAN<<17);	

	msg.length = 6;

	msg.data[0] = device.uid >> 24;
	msg.data[1] = device.uid >> 16;
	msg.data[2] = device.uid >> 8;
	msg.data[3] = device.uid;
	msg.data[4] = CMD_BOOTSUB_BLKN;	
	msg.data[5] = num;

//	print_can_hex_detailed(&msg);
//	return(0);
	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send 0x1B command up to 1024 Bytes data stream
//
// Details: The start hash value of 0x0300 is the byte number and is incremented
// on every sended Byte
// mode == 0 reset hash value to 0x300
// 
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
snd_binStream(char *bytes, uint16_t len, uint8_t mode)	{

	uint8_t i = 0;
	uint16_t j = 0;
	static uint16_t nbyte;
	can_t msg;	

	if(! mode )
		nbyte = 0x300;

	//Build ID
	msg.length = 8;

	len = len / 8;

	for(j = 0; j < len; j++, nbyte++)	{

		for(i = 0; i < 8; i++)	{

			msg.id = 0;							
			msg.id |= nbyte;
			msg.id |= ((unsigned long)CMD_BOOTLD_CAN << 17);	
			msg.data[i] = *bytes++;
		}

//	print_can_hex_detailed(&msg);
		can_send_message(&msg);
	}

	return(0);
}

// -----------------------------------------------------------------------------
// Description: Send 0x1B command 16Bit CRC sum after data Stream
//
// Details: The CRC16 sum of previours send data with snd_binStream(). Must
// be ACK from receiver ( may take a while... )
// 
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
snd_binCRC(void)	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= UPD_HASH;			
	msg.id |= ((unsigned long)CMD_BOOTLD_CAN << 17);	

	msg.length = 7;

	msg.data[0] = device.uid >> 24;
	msg.data[1] = device.uid >> 16;
	msg.data[2] = device.uid >> 8;
	msg.data[3] = device.uid;
	msg.data[4] = CMD_BOOTSUB_CRC;
	msg.data[5] = my_crc >> 8;
	msg.data[6] = my_crc;

//	print_can_hex_detailed(&msg);
//	return(0);
	return(can_send_message(&msg));

}

// -----------------------------------------------------------------------------
// Description: Send 0x1B command as ACK for cf data Stream
//
// Details: UNEXPECTED Dazagram send after 0x21 config data stream finished
// 
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
snd_ACK(void)	{

	can_t msg; 
	
	//Build ID
	msg.id = 0;							
	msg.id |= device.hash;			
	msg.id |= ((unsigned long)CMD_BOOTLD_CAN<<17);	

	msg.length = 6;
		
	msg.data[0] = 0;
	msg.data[1] = 0x2;
	msg.data[2] = 0;
	msg.data[3] = 0;
	msg.data[4] = 0;
	msg.data[5] = 0;

	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send 0x21 command contains lengnt of following stream and CRC
//
// Details: Befor a config stream can be send, its length in byte and a 16 Bit
// CRC sum must be send
//
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
snd_cfCRC(uint16_t len)	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= device.hash;		
	msg.id |= ((unsigned long)CMD_CFG_STREAM << 17);	

	msg.length = 6;

	msg.data[0] = 0;
	msg.data[1] = 0;
	msg.data[2] = len >> 8;
	msg.data[3] = len;

	msg.data[4] = my_crc >> 8;
	msg.data[5] = my_crc;

//	print_can_hex_detailed(&msg);
//	return(0);
	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send 0x21 command with 8 Btye data 
//
// Details: After 0x20 request/response a 0x21 Dataset can be send. It contains
// allways 8 data byte. Empty bytes must be stuffed by 0x00. Len must be size
// of power of 2 !
//
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
snd_cfStream(char *bytes, uint16_t len)	{

	uint8_t i = 0;
	uint16_t j = 0;
	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= device.hash;		
	msg.id |= ((unsigned long)CMD_CFG_STREAM << 17);	

	msg.length = 8;

	len = len / 8;

	for(j = 0; j < len; j++)	{

		for(i = 0; i < 8; i++)	{

			msg.data[i] = *bytes++;
		}

//		print_can_hex_detailed(&msg);
		can_send_message(&msg);
	}

	return(0);
}

// -----------------------------------------------------------------------------
// Description: Send 0x18 request command 
//
// Details: After sending 0x18, all devices connected to the CAN-Bus
// must reply. See details at "CS2-CAN-Protokoll" description.
//
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
snd_ping(void)	{

	can_t msg; 
	
	//Build ID
	msg.id = 0;							
	msg.id |= UPD_HASH;			
	msg.id |= ((unsigned long)CMD_PING<<17);	

	msg.length = 0;
		
	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send 0x18 response command 
//
// Details: Response appropiate data to previours received 0x18 request.
// See details at "CS2-CAN-Protokoll" description.
//
// Response with previous received hash initiate update procedure!
//
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
resp_ping(uint16_t hash)	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= hash;		
	msg.id |= ((unsigned long)RESPONSE << 16);	
	msg.id |= ((unsigned long)CMD_PING << 17);	

	msg.length = 8;
		
	msg.data[0] = UID0;
	msg.data[1] = UID1;
	msg.data[2] = UID2;
	msg.data[3] = UID3;
	msg.data[4] = SVERS0;
	msg.data[5] = SVERS1;
	//Magic device ID for CS2-GUI Master
	msg.data[6] = 0xFF;
	msg.data[7] = 0xFF;

	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Respons a config request
//
// Details: Response appropiate data to previours received 0x20 request.
// See details at "CS2-CAN-Protokoll" description.
//
// Called by: 
//
// Return: address of used tx buffer on success otherwise -1
// -----------------------------------------------------------------------------
int
resp_cfg_request(uint8_t cfgName[8])	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= UPD_HASH;		
	msg.id |= ((unsigned long)RESPONSE << 16);	
	msg.id |= ((unsigned long)CMD_CFG_REQUEST << 17);	

	msg.length = 8;
		
	msg.data[0] = cfgName[0];
	msg.data[1] = cfgName[1];
	msg.data[2] = cfgName[2];
	msg.data[3] = cfgName[3];
	msg.data[4] = cfgName[4];
	msg.data[5] = cfgName[5];
	msg.data[6] = cfgName[6];
	msg.data[7] = cfgName[7];

//	print_can_hex_detailed(&msg);
	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send system sub cmd 0x80 system reset
//
// Details: Internal target of addressed device ID is fixed to 0xFF.
// Target value is not documented
//
// Called by: 
//
// Return: void
// -----------------------------------------------------------------------------
int
snd_sysReset(void)	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= UPD_HASH;		
	msg.id |= ((unsigned long)CMD_SYSTEM << 17);	

	msg.length = 6;

	msg.data[0] = device.uid >> 24;
	msg.data[1] = device.uid >> 16;
	msg.data[2] = device.uid >> 8;
	msg.data[3] = device.uid;
	msg.data[4] = CMD_SYSSUB_RESET;
	msg.data[5] = 0xFF;

	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send 0x1B command
//
// Details: Send DLC = 0 will invoce to start loaded software
//
// Called by: 
//
// Return: void
// -----------------------------------------------------------------------------
int
snd_bootInit(void)	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= UPD_HASH;		
	msg.id |= ((unsigned long)CMD_BOOTLD_CAN << 17);	

	msg.length = 0;

	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: Send 0x1B command
//
// Details: Send DLC = 5 with 4 = 0x11 will invoce a system restart
//
// Called by: 
//
// Return: void
// -----------------------------------------------------------------------------
int
snd_bootStart(void)	{

	can_t msg;	

	//Build ID
	msg.id = 0;							
	msg.id |= UPD_HASH;		
	msg.id |= ((unsigned long)CMD_BOOTLD_CAN << 17);	

	msg.length = 5;

	msg.data[0] = device.uid >> 24;
	msg.data[1] = device.uid >> 16;
	msg.data[2] = device.uid >> 8;
	msg.data[3] = device.uid;
	//msg.data[4] = 0xF5; // AIIEEE THAS realy Magic! Seen this CS2 -> MS2, but not MS2->MS2

	//can_send_message(&msg);
	msg.data[4] = CMD_BOOTSUB_START;
	return(can_send_message(&msg));
}

// -----------------------------------------------------------------------------
// Description: calculate CRC of given data until length defined by len
//
// Details: The result is stored to global my_crc value
// Mode == 0 init local my_crc, 1 == update my_crc
//
// Called by: 
//
// Return: void
// -----------------------------------------------------------------------------
void
create_CRC(char *data, uint16_t len, uint8_t mode)	{

	uint16_t i = 0;

	if(! mode)
		my_crc = 0xFFFF;

	for( i = 0; i < len; i++)	{
		update_crc(data[i]);
	}
}


// -----------------------------------------------------------------------------
// Description: calculate CRC for one byte and update crc value
//
// Details: Using CRC calculation documented by Mrklin
//
// Called by: 
//
// Return: void
// -----------------------------------------------------------------------------
static void
update_crc(char ch) {

	uint8_t i = 0;

	my_crc = my_crc ^ (ch << 8);

	for (i = 0; i < 8; i++) {

		if ((my_crc & 0x8000) == 0x8000) {
			my_crc = my_crc << 1;
			my_crc ^= POLY;
		} else {
			my_crc = my_crc << 1;
		}
	}
}
