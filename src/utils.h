/*
* ----------------------------------------------------------------------------
* Macro interface to PORT and PIN
*
* Origin unknown but hopfully open source...
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef	UTILS_H
#define	UTILS_H

#ifndef	TRUE
	#define	TRUE	0x01
	#define	FALSE	0x00
#endif

#define	PORT(x)			_port2(x)
#define	DDR(x)			_ddr2(x)
#define	PIN(x)			_pin2(x)

#define	RESET(x)		RESET2(x)
#define	SET(x)			SET2(x)
#define	TOGGLE(x)		TOGGLE2(x)
#define	SET_OUTPUT(x)	SET_OUTPUT2(x)
#define	SET_INPUT(x)	SET_INPUT2(x)
#define	SET_PULLUP(x)	SET2(x)
#define	IS_SET(x)		IS_SET2(x)

#define	RESET2(x,y)		PORT(x) &= ~(1<<y)
#define	SET2(x,y)		PORT(x) |= (1<<y)
#define	TOGGLE2(x,y)	PORT(x) ^= (1<<y)

#define	SET_OUTPUT2(x,y)	DDR(x) |= (1<<y)
#define	SET_INPUT2(x,y)		DDR(x) &= ~(1<<y)

#define	IS_SET2(x,y)	((PIN(x) & (1<<y)) != 0)

#define	_port2(x)	PORT ## x
#define	_ddr2(x)	DDR ## x
#define	_pin2(x)	PIN ## x

#endif	
