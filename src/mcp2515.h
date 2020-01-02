/*
* ----------------------------------------------------------------------------
* Low level driver for MCP2515 CAN chip
*
* Based on Fabian Greif CAN lib
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef MCP2515_H
#define MCP2515_H

#if defined (__cplusplus)
	extern "C" {
#endif
		
#include <stdbool.h>

// ----------------------------------------------------------------------------
typedef struct
{
	uint32_t id;			//!< ID 29 Bit)
	uint8_t length;			//!< data byte length
	uint8_t data[8];		//!< data of CAN message
} can_t;


// ----------------------------------------------------------------------------
typedef enum {
	LISTEN_ONLY_MODE,		//!< Listen only, total passive
	LOOPBACK_MODE,			//!< Loopback all messages without sending
	NORMAL_MODE,			//!< Normal mode
} can_mode_t;

// ----------------------------------------------------------------------------
bool
can_init(void);

// ----------------------------------------------------------------------------
uint8_t
can_get_message(can_t *msg);

// ----------------------------------------------------------------------------
uint8_t
can_send_message(const can_t *msg);

// ----------------------------------------------------------------------------
void
can_set_mode(can_mode_t mode);

// -------------------------------------------------------------------------
void
can_write_register( uint8_t adress, uint8_t data);

// -------------------------------------------------------------------------
uint8_t
can_read_register(uint8_t adress);

// -------------------------------------------------------------------------
uint8_t
can_read_status(uint8_t type);

// -------------------------------------------------------------------------
void
can_bit_modify(uint8_t adress, uint8_t mask, uint8_t data);

// -------------------------------------------------------------------------
void
can_write_id( const uint32_t *id);

// -------------------------------------------------------------------------
uint8_t
can_read_id( uint32_t *id );


#if defined (__cplusplus)
}
#endif

#endif // CAN_H
