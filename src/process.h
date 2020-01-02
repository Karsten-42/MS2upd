/*
* ----------------------------------------------------------------------------
* Processing software update to Märklin devices via CAN bus
*
*  Version: 0.0.1
*  
* "THE BEER-WARE LICENSE" (Revision 42):
* <karsten@rhen.de> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in Flensburg, Germany
* ----------------------------------------------------------------------------
*/

#ifndef PROCESS_H
#define PROCESS_H


uint8_t init_60113(device_t *device);
uint8_t init_MS2(device_t *device);

uint16_t get_gb2ver(char *fName);
uint16_t get_ms2ver(char *fName);

void process_sysReset(void);
uint8_t process_bootInit(void);
uint8_t process_binBlock(uint8_t num);
uint8_t process_binCRC(void);
uint8_t process_bintransfer(char *fName, uint16_t blksize, uint8_t magic);

uint8_t process_60133Update(void);
uint8_t process_MS2Update(device_t *device);

// Times and detection
#define ENOMS2		1	// No MS2 found	
#define ENOGFP		2	// No 60113 box found
#define ETIMED		9	// Time out
#define ENOFILE	23 // No such file

// State of dispatched functions
#define FLASHUPD			0xFE	// Invoce a flash update
#define REBOOT				0xFF	// Invoce a reboot 

#endif


