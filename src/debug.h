/*
* ----------------------------------------------------------------------------
* Constants für simple UART debugging messages
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef MYDEBUG_H
#define MYDEBUG_H

#define DEBUG 1

#ifdef DEBUG
#define	PRINT(string, ...)		printf_P(PSTR(string), ##__VA_ARGS__)
#else
#define	PRINT(string, ...)	do{} while(0)
#endif


// -----------------------------------------------------------------------------
// Global file handle
extern FILE mystdout;

// -----------------------------------------------------------------------------
// redirection to UART
int putchar__(char c, FILE *stream);

// -----------------------------------------------------------------------------
// Print CAN msg 
void print_can_hex_detailed(can_t *msg);

#endif



