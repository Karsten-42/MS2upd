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
* ----------------------------------------------------------------------------
*
* Original from rocrail source from a time it was open and free....
* 
* Maerklin CAN commands transport within 29Bit CAN header
*
* Format:
* CAN ID  29 bit header. MSB3, msb7,6,5 take not in account
* <---3--><---2--><---1--><---0-->
* 10987654321098765432109876543210
* pppppppccccccccrhhhhhhhhhhhhhhhh
*  
*  p = prio 4 bit
*  c = command 8bit
*  r = Response bit
*  h = Hash 16 Bit.
*
*  Hash must be
*  <---2--><---1-->
*  xxxxxx110xxxxxxx
*
*  x = MSB-UID ^ LSB-UID
*
*	Version 0.5  based on Maerklin CAN documentation v2.0 07.02.2012
*
*/


#ifndef MCS2CONST_H_
#define MCS2CONST_H_

/* GLOBAL */
#define RESPONSE					0x01

/* SYSTEM */
#define CMD_SYSTEM            0x00
#define ID_SYSTEM             0x00
#define CMD_SYSSUB_STOP       0x00
#define CMD_SYSSUB_GO         0x01
#define CMD_SYSSUB_HALT       0x02
#define CMD_SYSSUB_EMBREAK    0x03
#define CMD_SYSSUB_STOPCYCLE  0x04
#define CMD_SYSSUB_LOCOPROT	0x05
#define CMD_SYSSUB_SWTIME		0x06
#define CMD_SYSSUB_FASTREAD	0x07 // Be carefully, CMD_LOCO_VERIFY first! 
#define CMD_SYSSUB_GFPROTO		0x08
#define CMD_SYSSUB_NEWREGNR   0x09
#define CMD_SYSSUB_OVERLOAD   0x0A // can only be recieved!
#define CMD_SYSSUB_STATUS     0x0B
#define CMD_SYSSUB_SET88ID    0x0C
#define CMD_SYSSUB_MFXSEEK    0x30	// No documentation, DONT USE IT
#define CMD_SYSSUB_RESET		0x80



/* LOCOs */
#define CMD_LOCO_DISCOVERY    0x01
#define ID_LOCO_DISCOVERY     0x02

#define CMD_LOCO_BIND         0x02
#define ID_LOCO_BIND          0x04

#define CMD_LOCO_VERIFY       0x03
#define ID_LOCO_VERIFY        0x06

#define CMD_LOCO_VELOCITY     0x04
#define ID_LOCO_VELOCITY      0x08

#define CMD_LOCO_DIRECTION    0x05
#define ID_LOCO_DIRECTION     0x0A

#define CMD_LOCO_FUNCTION     0x06
#define ID_LOCO_FUNCTION      0x0C

#define CMD_LOCO_READ_CONFIG  0x07
#define ID_LOCO_READ_CONFIG   0x0E

#define CMD_LOCO_WRITE_CONFIG 0x08
#define ID_LOCO_WRITE_CONFIG  0x10


/* ACCESSORIES */
#define CMD_ACC_SWITCH        0x0B
#define ID_ACC_SWITCH         0x16
#define CMD_ACC_SENSOR        0x11

#define CMD_ACC_CONFIG			0x1D
#define ID_ACC_CONFIG			0x3A

/* CONFIG DATA */
#define CMD_CFG_REQUEST			0x20
#define ID_CFG_REQUEST			0x40
#define CMD_CFG_STREAM			0x21
#define ID_CFG_STREAM			0x42

/* SOFTWARE */
#define CMD_PING         0x18
#define ID_PING          0x30

#define CMD_BOOTLD_CAN		0x1B
#define ID_BOOTLD_CAN      0x36

#define CMD_BOOTSUB_START	0x11	// Start command
#define CMD_BOOTSUB_BLKN	0x44	// Block number
#define CMD_BOOTSUB_CRC		0x88	// CRC of previours sent block
#define CMD_BOOTSUB_REBOOT	0xF5	// initiate a Reboot ( MS2 )

/* Hardware */
#define DEV_GFP_CS2	0x00		// 60214(CS2) or 60174(Booster)
#define DEV_GFP_MS2	0x10		// GFP for MS2 "Gleisbox" 60113
#define DEV_CON_UNI	0x20		// Control unit 60128
#define DEV_CON_MS2	0x32		// Control unit MS2
#define DEV_DEV_WLA	0xE0		// Wireless devices
#define DEV_MST_CS2	0xFF		// CS2-GUI ( Master )


#endif /* MCS2CONST_H_ */
