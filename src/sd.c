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

#include "sd.h"

// -----------------------------------------------------------------------------
//  INTERFACE to MMC-lib 
// -----------------------------------------------------------------------------

FATFS fs;
FIL fd;

// -----------------------------------------------------------------------------
// Stub: Init access to SD Card
//
// Called by: diverse
//
// Return: 0 on success othervise appropiate errno
// ----------------------------------------------------------------------------
uint8_t
init_SD(void)	{

	return(f_mount(&fs, "0:", 1));
}


// -----------------------------------------------------------------------------
// Stub: Open given filename in current directory
//
// Details: search current opened dir_ent for given filename. 
// The file name string must be terminated by \0 .
// 
// !! Open READ-ONLY !!
//
// Called by: diverse
//
// Return: 0 on success othervise appropiate errno
// ----------------------------------------------------------------------------
uint8_t
sd_open_file(char *filename)	{

	return(f_open(&fd, filename, FA_READ));
}

// -----------------------------------------------------------------------------
// Stub: close SD card file descriptor
//
// Called by: diverse
//
// Return: void
// ----------------------------------------------------------------------------
void
sd_close_file(void)	{

	f_close(&fd);
}

// -----------------------------------------------------------------------------
// Stub: seek to file position from absolue file start position
//
// Details: seek is fixed to SEEK_SET
//
// Called by: diverse
//
// Return: void, update *pos with curr file pos after seek
// ----------------------------------------------------------------------------
uint8_t
sd_seek_file(uint32_t *pos)	{

	if(f_lseek(&fd, *pos))
		return(1);

	*pos = f_tell(&fd);
	return(0);
}

// -----------------------------------------------------------------------------
// Stub: Return current file position from absolue file start position
//
// Called by: diverse
//
// Return: Cur file position
// ----------------------------------------------------------------------------
uint32_t
sd_tell_file(void)	{

	return(f_tell(&fd));
}

// -----------------------------------------------------------------------------
// Stub: read data from opened file
//
// Details: buffer must be large enought for given lenght
//
// Called by: diverse
//
// Return: The number of bytes read, 0 on end of file, or -1 on failure. 
// ----------------------------------------------------------------------------
uint16_t
sd_read_file(uint8_t *buffer, uint16_t len)	{

	uint16_t rd;

	f_read(&fd, buffer, len, &rd);
	return(rd);
}

// -----------------------------------------------------------------------------
// Stub: return file size
//
// Called by: diverse
//
// Return: file size in bytes
// ----------------------------------------------------------------------------
uint32_t
sd_file_size(void)	{

	return(f_size(&fd));
}
