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
*/

#ifndef MAIN_H
#define MAIN_H

#include "spi.h"
#include "uart.h"
#include "sd.h"
#include "lcd.h"
#include "mcs2-const.h"
#include "protocol.h"
#include "mcp2515.h"
#include "process.h"
#include "config.h"
#include "debug.h"

// -----------------------------------------------------------------------------
// Global timeout timing
extern volatile uint16_t count;

// -----------------------------------------------------------------------------
// Retrieve CAN rx buffer from buffer pool
bool read_rx_buffer(can_t *msg);
void clear_rx_buffer(void);

// -----------------------------------------------------------------------------
// CAN Buffer element size = 13 Byte
#define BUF_SIZE 4

// -----------------------------------------------------------------------------
// Start key debounced
#define KEY_INPUT PIND	//Port D as Key input
#define KEY_START PD7	//Pin 13
#define KEY D,7			//To set pull up 

// -----------------------------------------------------------------------------
// Timing number multiplied by 16ms
#define TMOCNT		4		// maximal message timeout 16ms * 4 = 64ms
#define DEVCCNT	30		// Ms2 Startmsg max time 16ms * 30 = 480ms
#define LOSTCNT	62		// Check for lost SD card 16ms * 62 = 992ms
#define DEVCNT		625	// Wait for MS2 boot  = 10s 
#define PNGCNT		150	// Wait for MS2 first ping = 2,4s

// -----------------------------------------------------------------------------
// Fast flags
#define Flags GPIOR0
enum {
	LOSTCARD,	// Card not present, wait for insert
	DEVBOOT,		// Wait for Device boot
	DEVCALC,	   // Device calculating timeout
	TIMEOUT,		// Message timeout
	FIRSTPNG,	// wait for first ping of device
	RESERVED5,
	RESERVED6,
	RESERVED7
};


#endif
