/*
* ----------------------------------------------------------------------------
* High level implementation to SD card
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef SD_H
#define SD_H

#include "mmc.h"
#include "ff.h"
#include "diskio.h"

// ----------------------------------------------------------------------------
uint8_t init_SD(void);

// ----------------------------------------------------------------------------
uint8_t sd_open_file(char *filename);

// ----------------------------------------------------------------------------
uint8_t sd_seek_file(uint32_t *pos);

// ----------------------------------------------------------------------------
uint32_t sd_tell_file(void);

// ----------------------------------------------------------------------------
void sd_close_file(void);

// ----------------------------------------------------------------------------
uint16_t sd_read_file(uint8_t *buffer, uint16_t len);

// ----------------------------------------------------------------------------
uint32_t sd_file_size(void);

#endif

