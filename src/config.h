/*
* ----------------------------------------------------------------------------
* Major configuration for -Reactivator- Märklin devices
*
* Configure HERE some... the Hardware
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef CONFIG_H
#define CONFIG_H

// TODO   Should be only ONE Hardware config file....

#include "utils.h"

// -----------------------------------------------------------------------------
// Hardware I/O Ports
#define LED_ERROR		D,5
#define LED_MSG		D,6
#define DEV_PRESENT  C,5	// Input for device detect
#define SW_START		D,7



#endif
