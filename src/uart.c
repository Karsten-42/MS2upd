/*
* ----------------------------------------------------------------------------
* Very simple unbuffered interface to Serial UART
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#include "uart.h"

// -----------------------------------------------------------------------------
// Description: Send one char and wait until UART send it
//
// Called by: div 
//
// Return: void
// -----------------------------------------------------------------------------
void
uart_putc(unsigned char data)	{

	loop_until_bit_is_set(UART0_STATUS, UART0_UDRE);
	UART0_DATA = data;								
}	

// -----------------------------------------------------------------------------
// Description: Send a string and wait until UART send it
//
// Details: String MUST be terminated with '\0' !
//
// Called by: div 
//
// Return: void
// -----------------------------------------------------------------------------
void
uart_puts(const char *s)	{	
	
	while(*s) uart_putc(*s++);
  
}

// -----------------------------------------------------------------------------
// Description: read one char and wait until UART read all bytes
//
// Details: This is BLOCKING!
//
// Called by: div 
//
// Return: unsigned char
// -----------------------------------------------------------------------------
unsigned char
uart_getc(void)	{

  while (!(UART0_STATUS & (1<<UART0_RXC))) {;}	
        
  return UART0_DATA; 
}

// -----------------------------------------------------------------------------
// Description: Initialize UART and set baudrate. Slow but running :-)
//
// Details: Need nice UART_BAUD_SELECT() macro, NO interrupt handling!
//
// Called by: div 
//
// Return: void
// -----------------------------------------------------------------------------
void
uart_init(unsigned int baudrate)	{	

#if defined( AT90_UART )
    /* set baud rate */
    UBRR = (unsigned char)baudrate; 

    /* enable UART receiver and transmmitter  */
    UART0_CONTROL = _BV(RXEN)|_BV(TXEN);

#elif defined (ATMEGA_USART)
    /* Set baud rate */
    if ( baudrate & 0x8000 )
    {
    	 UART0_STATUS = (1<<U2X);  //Enable 2x speed 
    	 baudrate &= ~0x8000;
    }
    UBRRH = (unsigned char)(baudrate>>8);
    UBRRL = (unsigned char) baudrate;
   
    /* Enable USART receiver and transmitter  */
    UART0_CONTROL = (1<<RXEN)|(1<<TXEN);
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
    #ifdef URSEL
    UCSRC = (1<<URSEL)|(3<<UCSZ0);
    #else
    UCSRC = (3<<UCSZ0);
    #endif 
    
#elif defined (ATMEGA_USART0 )
    /* Set baud rate */
    if ( baudrate & 0x8000 ) 
    {
   		UART0_STATUS = (1<<U2X0);  //Enable 2x speed 
   		baudrate &= ~0x8000;
   	}
    UBRR0H = (unsigned char)(baudrate>>8);
    UBRR0L = (unsigned char) baudrate;

    /* Enable USART receiver and transmitter */
    UART0_CONTROL = (1<<RXEN0)|(1<<TXEN0);
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
    #ifdef URSEL0
    UCSR0C = (1<<URSEL0)|(3<<UCSZ00);
    #else
    UCSR0C = (3<<UCSZ00);
    #endif 

#elif defined ( ATMEGA_UART )
    /* set baud rate */
    if ( baudrate & 0x8000 ) 
    {
    	UART0_STATUS = (1<<U2X);  //Enable 2x speed 
    	baudrate &= ~0x8000;
    }
    UBRRHI = (unsigned char)(baudrate>>8);
    UBRR   = (unsigned char) baudrate;

    /* Enable UART receiver and transmitter */
    UART0_CONTROL = (1<<RXEN)|(1<<TXEN);

#endif

}
