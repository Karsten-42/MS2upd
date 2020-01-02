/*
* ----------------------------------------------------------------------------
* Debug functions for simple debug messages via UART
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
#include <avr/pgmspace.h>
#include "uart.h"
#include "mcp2515.h"
#include "debug.h"

// -----------------------------------------------------------------------------
// global file handle
FILE mystdout = FDEV_SETUP_STREAM(putchar__, 0, _FDEV_SETUP_WRITE);

// -----------------------------------------------------------------------------
// redirection to UART
int putchar__(char c, FILE *stream) {
	uart_putc(c);
	return 0;
}

// -----------------------------------------------------------------------------
// Description: Print the received message to RS232 in detailed format
//
// Called by: div 
//
// Return: void
// -----------------------------------------------------------------------------
void
print_can_hex_detailed(can_t *msg)	{

	uint8_t tmp = 0;
	uint16_t xtmp = 0;
	
	tmp = (msg->id >> 17) & 0xFF;
	PRINT("C:0x%02X ", tmp);

	tmp = (msg->id >> 16) & 1;
	PRINT("R:%d ",tmp);

	xtmp = msg->id & 0xFFFF;
	PRINT("H:0x%04X ",xtmp);

	tmp = msg->length;
	PRINT("D:%d ",tmp);
	
	switch(tmp)	{

		case 0:
			PRINT("\n");
			break;
		case 1:
			PRINT("D:0x%02x \n",
					msg->data[0]);
			break;
		case 2:
			PRINT("D:0x%02x 0x%02x \n",
					msg->data[0],
					msg->data[1]);
			break;
		case 3:
			PRINT("D:0x%02x 0x%02x 0x%02x \n",
					msg->data[0],
					msg->data[1],
					msg->data[2]);
			break;
		case 4:
			PRINT("D:0x%02x 0x%02x 0x%02x 0x%02x \n",
					msg->data[0],
					msg->data[1],
					msg->data[2],
					msg->data[3]);
			break;
		case 5:
			PRINT("D:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
					msg->data[0],
					msg->data[1],
					msg->data[2],
					msg->data[3],
					msg->data[4]);
			break;
		case 6:
			PRINT("D:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
					msg->data[0],
					msg->data[1],
					msg->data[2],
					msg->data[3],
					msg->data[4],
					msg->data[5]);
			break;
		case 7:
			PRINT("D:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
					msg->data[0],
					msg->data[1],
					msg->data[2],
					msg->data[3],
					msg->data[4],
					msg->data[5],
					msg->data[6]);
			break;
		case 8:
			PRINT("D:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
					msg->data[0],
					msg->data[1],
					msg->data[2],
					msg->data[3],
					msg->data[4],
					msg->data[5],
					msg->data[6],
					msg->data[7]);
			break;
	}
}
