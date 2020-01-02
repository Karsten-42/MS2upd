/*
* ----------------------------------------------------------------------------
* Low level driver vom MCP2515 CAN chip
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
*
* Base on Fabian Greif CAN lib
*
* ----------------------------------------------------------------------------
*/


#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "spi.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"


// -------------------------------------------------------------------------
void can_write_register( uint8_t adress, uint8_t data )
{
	MCP_CS_LOW;

	spi_write_byte(SPI_WRITE);
	spi_write_byte(adress);
	spi_write_byte(data);
	
	MCP_CS_HIGH;
}


// -------------------------------------------------------------------------
uint8_t can_read_register(uint8_t adress)
{
	uint8_t data;
	
	MCP_CS_LOW;
	
	spi_write_byte(SPI_READ);
	spi_write_byte(adress);
	
	data = spi_read_byte();	
	
	MCP_CS_HIGH;
	
	return data;
}

// -------------------------------------------------------------------------
void can_bit_modify(uint8_t adress, uint8_t mask, uint8_t data)
{
	MCP_CS_LOW;
	
	spi_write_byte(SPI_BIT_MODIFY);
	spi_write_byte(adress);
	spi_write_byte(mask);
	spi_write_byte(data);
	
	MCP_CS_HIGH;
}

// ----------------------------------------------------------------------------
uint8_t can_read_status(uint8_t type)
{
	uint8_t data;
	
	MCP_CS_LOW;
	
	spi_write_byte(type);
	data = spi_read_byte();
	
	MCP_CS_HIGH;
	
	return data;
}

// -------------------------------------------------------------------------
/*************** MCP2515 init ******************/
bool can_init()
{

	// Reset MCP2515 lead to configure mode
	MCP_CS_LOW;
	spi_write_byte(SPI_RESET);
	_delay_ms(1);
	MCP_CS_HIGH;

	_delay_ms(10);			// restart MCP2515

	// CNF3 -> CNF1 load register (Bittiming)
	MCP_CS_LOW;
	spi_write_byte(SPI_WRITE);
	spi_write_byte(CNF3);

	// set CAN-Bus Bitrate to 250Kbit
	spi_write_byte((1<<PHSEG21));						//CNF3
	spi_write_byte((1<<BTLMODE)|(1<<PHSEG11));	//CNF2
	spi_write_byte((1<<BRP1)|(1<<BRP0));			//CNF1 Prescaler

	// activating interrup on RX both buffer
	spi_write_byte((1<<RX0IE)|(1<<RX1IE));			//CANINTE
	MCP_CS_HIGH;
	
	// TXnRTS Bits as input
	can_write_register(TXRTSCTRL, 0);

	// Deactivate RXnBF Pins (High Impedance State)
	can_write_register(BFPCTRL, 0);

	// Deactivate filters -> Receive any message, allow rollover to RXB1
	can_write_register(RXB0CTRL, (1<<RXM1)|(1<<RXM0)|(1<<BUKT));
	can_write_register(RXB1CTRL, (1<<RXM1)|(1<<RXM0));

	// Check if this MCP2515 is accessable, read prescaler value CNF1
	if (can_read_register(CNF1) != 3)
		return(false);

	// Set MCP2515 to run mode, disable clkout port
	can_write_register(CANCTRL, 0);

	// Wait for activating new mode
	while ((can_read_register(CANSTAT) & 0xe0) != 0);

	return(true);
}

// ----------------------------------------------------------------------------
uint8_t can_get_message(can_t *msg)
{
	uint8_t addr;

	// read status
	MCP_CS_LOW;
	spi_write_byte(SPI_RX_STATUS);
	uint8_t status = spi_read_byte();
	MCP_CS_HIGH;
	
	if (bit_is_set(status,6)) {
		// message in buffer 0
		addr = SPI_READ_RX;
	}
	else if (bit_is_set(status,7)) {
		// message in buffer 1
		addr = SPI_READ_RX | 0x04;
	}
	else {
		// no message available
		return 0;
	}
	
	MCP_CS_LOW;

	spi_write_byte(addr);
	can_read_id(&msg->id);
	
	// read DLC
	uint8_t length = spi_read_byte();
	length &= 0x0f;
	msg->length = length;

	// read data
	for (uint8_t i=0;i<length;i++) {
		msg->data[i] = spi_read_byte();
	}

	MCP_CS_HIGH;
	
	// clear interrupt flag
	if (bit_is_set(status, 6))
		can_bit_modify(CANINTF, (1<<RX0IF), 0);
	else
		can_bit_modify(CANINTF, (1<<RX1IF), 0);
	
	return (status & 0x07) + 1;
}

// ----------------------------------------------------------------------------
// Read ID from MCP2515 register
uint8_t can_read_id(uint32_t *id)
{
	uint8_t first;
	uint8_t tmp;
	
	first = spi_read_byte();
	tmp   = spi_read_byte();
	
	if (tmp & (1 << IDE)) {
		spi_start(0xff);
		
		*((uint16_t *) id + 1)  = (uint16_t) first << 5;
		*((uint8_t *)  id + 1)  = spi_wait();
		spi_start(0xff);
		
		*((uint8_t *)  id + 2) |= (tmp >> 3) & 0x1C;
		*((uint8_t *)  id + 2) |=  tmp & 0x03;
		
		*((uint8_t *)  id)      = spi_wait();
		
		return TRUE;
	}

	return FALSE;
}

void can_write_id(const uint32_t *id)
{
	uint8_t tmp;
	
	spi_start(*((uint16_t *) id + 1) >> 5);
	
	// naechsten Werte berechnen
	tmp  = (*((uint8_t *) id + 2) << 3) & 0xe0;
	tmp |= (1 << IDE);
	tmp |= (*((uint8_t *) id + 2)) & 0x03;
	
	// warten bis der vorherige Werte geschrieben wurde
	spi_wait();
	
	// restliche Werte schreiben
	spi_write_byte(tmp);
	spi_write_byte(*((uint8_t *) id + 1));
	spi_write_byte(*((uint8_t *) id));
}

uint8_t can_send_message(const can_t *msg)	{

	uint8_t address = 0;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)	{

		// Read status of MCP2515
		uint8_t status = can_read_status(SPI_READ_STATUS);
		
		/* Statusbyte:
		 *
		 * Bit	Funktion
		 *  2	TXB0CNTRL.TXREQ
		 *  4	TXB1CNTRL.TXREQ
		 *  6	TXB2CNTRL.TXREQ
		 */

		if (bit_is_clear(status, 2)) {
			address = 0x00;
		}
		else if (bit_is_clear(status, 4)) {
			address = 0x02;
		} 
		else if (bit_is_clear(status, 6)) {
			address = 0x04;
		}
		else {

			return 0;
		}
		
		MCP_CS_LOW;

		spi_write_byte(SPI_WRITE_TX | address);
		can_write_id(&msg->id);

		uint8_t length = msg->length & 0x0f;
		
		// set message length
		spi_write_byte(length);
			
		// send data via SPI
		for (uint8_t i=0;i<length;i++) {
			spi_write_byte(msg->data[i]);
		}

		MCP_CS_HIGH;
		_delay_us(1);
		
		// Send CAN message
		// The last three bit in RTS command point to
		// which buffer should send
		MCP_CS_LOW;

		address = (address == 0) ? 1 : address;
		spi_write_byte(SPI_RTS | address);

		MCP_CS_HIGH;
	}	

	return address;
}
