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

#include <avr/io.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "process.h"

#define BUFSIZE 32
#define BLOCKSIZE 1024	//Static fixed blocksize for 0x21 config data stream

// static prototyp
static void start_MS2(uint16_t hash);
static uint8_t update_MS2(void);

// prototypes for caller
uint8_t process_filever(char *fName);
uint8_t process_ldbver(char *fName);
uint8_t process_ms2ver(char *fName);
uint8_t process_gb2ver(char *fName);
uint8_t process_transfer(char *fName);


// Function caller definition
struct msg_fkt {

	uint8_t flashUpd;		// True if a binary flash update should be invoced
	uint8_t reqCount;		// number of sub requests related to this request
	char fktName[8];		// identify function by string
	char fName[12];		// appropiate file name
	uint8_t (*function)(char *fName);
};


struct msg_fkt caller[] = 	{

	{0,0, "langver","lang.ms2", process_filever},
	{0,1, "lang", "lang.ms2", process_transfer},	
	{0,0, "ldbver", "flashdb.ms2", process_ldbver},
	{0,1, "lokdb", "flashdb.ms2", process_transfer},
	{0,0, "ms2ver","050-ms2.bin", process_ms2ver},
	{1,1, "ms2","050-ms2.bin", process_transfer},
	{0,0, "ms2xver","051-ms2.bin", process_filever},
	{0,1, "ms2x","051-ms2.bin", process_transfer},
	{0,0, "gb2ver","016-gb2.bin", process_gb2ver},
	{0,1, "gb2","016-gb2.bin", process_transfer}
};

#define FKTCOUNT 10		// Number of functions in caller array
#define VERCOUNT	5		// Number of version check functions



// -----------------------------------------------------------------------------
// Description: Process a reset of target
//
// Details: Send system command reset with undocumented 0xFF to reset Target.
// This invoke a reboot of the given target. The target need approx 400ms
// to become ready to receive next CAN message.
// 
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
void
process_sysReset(void)	{

	snd_sysReset();

	TCNT0 = count = 0;
	Flags |= (1 << DEVCALC);
	while(Flags & (1 << DEVCALC));		// wait 400ms for reset target
}


// -----------------------------------------------------------------------------
// Description: Process the initialization of the bootloder
//
// Details: Send 0x1B boot loader init command and wait for ACK of target
//
// 
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_bootInit(void)	{

	uint8_t cmd = 0;
	can_t msg; 

	snd_bootInit();

	TCNT0 = count = 0;
	Flags |= (1 << TIMEOUT);
	while(Flags & (1 << TIMEOUT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_BOOTLD_CAN)	{
				return(0);
			}
		}
	}

	return(1);
}


// -----------------------------------------------------------------------------
// Description: Process the send of a block number and wait for ACK
//
// Details: Send 0x1B boot loader block number command. The block number must
// be decremented every call. ATTENTION: Magic last block number!!
// On MS2 binary last block Number is 0x4
// On 60113 binary last block number is 0x2
//
// 
// Called by: process_bintransfer();
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_binBlock(uint8_t num)	{

	uint8_t cmd = 0;
	can_t msg;

	snd_binBlock(num);

	TCNT0 = count = 0;
	Flags |= (1 << TIMEOUT);

	while(Flags & (1 << TIMEOUT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_BOOTLD_CAN ) {

				if(msg.data[5] == num)
					return(0);
				else
					return(1);
			}
		}
	}

	return(1);
}


// -----------------------------------------------------------------------------
// Description: Process the send of a 16Bit CRC number and wait for ACK
//
// Details: Send 0x1B boot loader CRC command and wait for ACK
// 
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_binCRC(void)	{

	uint8_t cmd = 0;
	can_t msg;

	snd_binCRC();

	TCNT0 = count = 0;
	Flags |= (1 << DEVCALC);

	while(Flags & (1 << DEVCALC))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_BOOTLD_CAN ) {

				if(msg.data[4] == CMD_BOOTSUB_CRC)	{
					return(0);
				}else	{
					return(1);
				}
			}
		}
	}

	return(1);
}

// -----------------------------------------------------------------------------
// Description: Process flash update of MS2
//
// Details: 
// 1) reset MS2
// 2) wait for MS2 boot init response
// 3) invoce binary transfer
// 4) MS2 init sequence
//
// 
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_flashUpd(void)	{

	uint8_t rt = 0;
	device_t device;

	process_sysReset();
	if((rt = process_bootInit())!= 0)	{

		//PRINT("bootInit timed out\n");
		return(rt);
	}

	if((rt = process_bintransfer("050-ms2.bin", 1024, MS2_BIN_MAGIC)) != 0)	{

		//PRINT("PANIC: Bin transfer rt = %d\n",rt);
		return(rt);
	}

	return(init_MS2(&device));
}

// -----------------------------------------------------------------------------
// Description: Process binary data transfer
//
// Details: Device must be initialize before. Read given last block size of 
// given file first. Calculate CRC and send the blocks and CRC per block.
// After last block is send restart the target.
//
// 
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_bintransfer(char *fName, uint16_t blksize, uint8_t magic)	{

	uint8_t blknum = 0, retval = 0, blkcnt = 0,j;
	uint16_t n = 0;
	uint32_t seek = 0;
	uint8_t buffer[BUFSIZE];

	if(sd_open_file(fName))	{

		PRINT("Cant open file %s\n",fName);
		return(1);
	}

	PRINT("Filesize:%d\n",sd_file_size());

	blknum = (sd_file_size() / blksize) + magic;
	seek = ((sd_file_size() / blksize) * blksize);

	for(n = 0; n < BUFSIZE; n++)
		buffer[n] = 0xFF;

	PRINT("Number of Blocks: %d Seek:%d\n",blknum,seek);

	// ANGST!
	while(1)	{

		sd_seek_file(&seek); //Seek to block position

		if((retval = process_binBlock(blknum--)) != 0)
			break;

		// read 32 Byte util block end
		while((n = sd_read_file(buffer, BUFSIZE)) > 0 )	{

			//0xFF padding
			if(n < BUFSIZE)	{
				for(j = n; j < BUFSIZE; j++)
					buffer[j] = 0xFF;
			}

			if(n % 8)
				n = ((n / 8) + 1) * 8;

			// update crc
			create_CRC((char *)buffer, n, blkcnt);

			// snd stream data
			snd_binStream((char *)buffer, n, blkcnt);

			blkcnt++;
			// whole block was read
			if((blksize / BUFSIZE) == blkcnt)	{
				break;
			}
		}

		if((retval = process_binCRC()) != 0)
			break;

		if(seek == 0)
			break;

		seek = seek - blksize;
		blkcnt = 0;
	}

	sd_close_file();
	snd_bootStart();

	return(retval);
}


// -----------------------------------------------------------------------------
// Description: Process file version of given file
//
// Details: Version information at Byte 1 and 2.
//
// Called by:  dispatcher
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_filever(char *fName)	{

	uint8_t i = 0;
	uint8_t vers[2];
	char bytes[48];

	PRINT("process_filever %s called\n",fName);
	if(sd_open_file(fName))	{

		return(ENOFILE);
	}

	sd_read_file(vers, 2);
	sd_close_file();

	// init buffer
	for(i = 0; i < 48; i++)
		bytes[i] = 0;

	// build command string
	sprintf(bytes," .vhigh=%d\n .vlow=%d\n .bytes=%ld\n",
			vers[0],
			vers[1],
			sd_file_size());

	i = ((strlen(bytes) / 8 ) + 1) * 8;
	create_CRC(bytes, i,0);

	snd_cfCRC(strlen(bytes));
	snd_cfStream(bytes, i);
	//snd_ACK();
	
	TCNT0 = count = 0;
	return(0);
}


// -----------------------------------------------------------------------------
// Description: Process file version flashdb file
//
// Details: Version information at Byte 1, Month at Byte 3
//
// Called by: dispatcher
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_ldbver(char *fName)	{

	uint8_t i = 0;
	uint8_t vers[4];
	char bytes[56];

	PRINT("process_ldbver %s called\n",fName);
	if(sd_open_file(fName))	{

		PRINT("Cant open File %s !\n",fName);
		return(ENOFILE);
	}

	sd_read_file(vers, 4);

	// init buffer
	for(i = 0; i < 56; i++)
		bytes[i] = 0;

	// build command string
	sprintf(bytes," .version=%d\n .monat=%d\n .jahr=20%d\n .anzahl=%ld\n",
			vers[0],
			vers[2],
			vers[0],
			((sd_file_size() /64 ) -1));

	sd_close_file();

	i = ((strlen(bytes) / 8 ) + 1) * 8;
	create_CRC(bytes, i,0);

	snd_cfCRC(strlen(bytes));
	snd_cfStream(bytes, i);
	//snd_ACK();
	
	TCNT0 = count = 0;
	return(0);
}


// -----------------------------------------------------------------------------
// Description: Process file transfer via config data sequence
//
// Details: 
// 1) Wait for block number request
// 2) Send CRC of block first. Blocksize fixed to 1024 Byte
// 3) Sending block data
// 4) Wait for ACK with name of requested config data
//
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_transfer(char *fName)	{

	uint8_t cmd = 0, blknum = 0, blkcnt = 0, i = 0, n = 0, j = 0;
	uint16_t rdbyte = 0;
	uint32_t fpos = 0, seek = 0, bytes = 0;
	uint8_t buffer[BUFSIZE];
	can_t msg;

	PRINT("process_transfer %s called\n",fName);

	if(sd_open_file(fName))	{

		PRINT("Cant open File %s !\n",fName);
		return(ENOFILE);
	}

	blknum = ((sd_file_size() / BLOCKSIZE) + 1);
	PRINT("%d blocks for %ld bytes!\n",blknum,sd_file_size());
//	PRINT("Blk req 0 OK, transfer ");

	for(blkcnt = 0; blkcnt < blknum; blkcnt ++)	{

		if(blkcnt > 0)	{ // Block #0 already responded by dispatcher!
			
			// wait for request of next block
			TCNT0 = count = cmd = rdbyte = 0;
			Flags |= (1 << DEVCALC);

			while(Flags & (1 << DEVCALC))	{

				if(read_rx_buffer(&msg))	{

					cmd = (msg.id >> 17) & 0xFF;

//					PRINT("Blk req ");
					if(cmd == CMD_CFG_REQUEST)	{
						//TODO
						// get block counter and chek it?
						// block ASCII decimal 61
						//C:0x20 R:0 H:0x1F64 D:8 D:0x36 0x31 0x00 0x00 0x00 0x00 0x00 0x00

						// reply block request
						resp_cfg_request(msg.data);
//						PRINT("%d OK, transfer ",blkcnt);
						break;
					}
				}
			}

			if(cmd == 0)	{

				PRINT("No Blockrequest in time, abort\n");
				sd_close_file();
				return(ETIMED);
			}
		 }

		// read one block in buffer size steps to calculate CRC
		for(i = 0; i < (BLOCKSIZE / BUFSIZE); i++)	{

			n = sd_read_file(buffer, BUFSIZE);

			rdbyte += n;
			if(n == 0) // End of file
				break;
			
			//0x00 padding
			if(n < BUFSIZE)	{
				for(j = n; j < BUFSIZE; j++)
					buffer[j] = 0x00;
			}

			if(n % 8)
				n = ((n / 8) + 1) * 8;

			create_CRC((char *)buffer, n, i);
		}

		snd_cfCRC(rdbyte);

		// rewind fd to block pos in file
		fpos = sd_tell_file();
		seek = fpos - rdbyte;
		sd_seek_file(&seek);

		// read one block in buffer size steps 
		for(i = 0; i < (BLOCKSIZE / BUFSIZE); i++)	{

			n = sd_read_file(buffer, BUFSIZE);
			
			if(n == 0) // End of file
				break;

			//0x00 padding
			if(n < BUFSIZE)	{
				for(j = n; j < BUFSIZE; j++)
					buffer[j] = 0x00;
			}

			if(n % 8)
				n = ((n / 8) + 1) * 8;

			snd_cfStream((char *)buffer, n);
		}

		//DEBUG
		bytes += rdbyte;
//		PRINT("Block#%d snd %d Btyes..%ld total",blkcnt,rdbyte,bytes);

		TCNT0 = count = cmd = 0;
		Flags |= (1 << DEVCALC);

		while(Flags & (1 << DEVCALC))	{

			if(read_rx_buffer(&msg))	{

				cmd = (msg.id >> 17) & 0xFF;

				if(cmd == CMD_CFG_REQUEST)	{

//					PRINT(" Req Cfg OK\n");
					break;
				}
			}
		}

		if(cmd == 0)	{

			if(blkcnt + 1 == blknum){	//last block  was send
//				PRINT("\nLast block no further request\n");
				break;
			}else	{
//				PRINT("\nNo config request in time, abort\n");
				sd_close_file();
				return(ETIMED);
			}
		}
	}

	PRINT("\n DONE OK\n");
	sd_close_file();
	TCNT0 = count = 0;

	return(REBOOT);
}

// -----------------------------------------------------------------------------
// Description: Process file version ms2 binary file
//
// Details: Version information at Byte 0xFC and 0xFD
//
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_ms2ver(char *fName)	{

	uint8_t i = 0;
	uint8_t vers[2];
	uint32_t seek = 0xFC;	// Magic position of 2 Byte version information
	char bytes[48];

	PRINT("process_ms2ver %s called\n",fName);
	if(sd_open_file(fName))	{

		return(ENOFILE);
	}

	sd_seek_file(&seek);

	sd_read_file(vers, 2);

	// init buffer
	for(i = 0; i < 48; i++)
		bytes[i] = 0;

	// build command string
	sprintf(bytes," .vhigh=%d\n .vlow=%d\n .bytes=%ld\n",
			vers[0],
			vers[1],
			sd_file_size());

	sd_close_file();

	i = ((strlen(bytes) / 8 ) + 1) * 8;
	create_CRC(bytes, i,0);

	snd_cfCRC(strlen(bytes));
	snd_cfStream(bytes, i);
	//snd_ACK();
	
	TCNT0 = count = 0;
	return(0);
}

// -----------------------------------------------------------------------------
// Description: Process file version 60133 binary file
//
// Details: Version information at Byte 7 and 8
//
// Called by: 
//
// Return: 0 on success otherwise errno 
// -----------------------------------------------------------------------------
uint8_t
process_gb2ver(char *fName)	{

	uint8_t i = 0;
	uint8_t vers[8];
	char bytes[48];

	PRINT("process_gb2ver %s called\n",fName);
	if(sd_open_file(fName))	{

		return(ENOFILE);
	}

	sd_read_file(vers, 8);

	// init buffer
	for(i = 0; i < 48; i++)
		bytes[i] = 0;

	// build command string
	sprintf(bytes," .vhigh=%d\n .vlow=%d\n .bytes=%ld\n",
			vers[6],
			vers[7],
			sd_file_size());

	sd_close_file();

	i = ((strlen(bytes) / 8 ) + 1) * 8;
	create_CRC(bytes, i,0);

	snd_cfCRC(strlen(bytes));
	snd_cfStream(bytes, i);
	//snd_ACK();
	
	TCNT0 = count = 0;
	return(0);
}

// -----------------------------------------------------------------------------
// Description: Retrieve 60133 binary file version
//
// Details: Version information at Byte 7 and 8
// 
// Called by: divers
//
// Return: 0 on error
// -----------------------------------------------------------------------------
uint16_t
get_gb2ver(char *fName)	{

	uint16_t i = 0;
	uint8_t vers[9];

	if(sd_open_file(fName))	{

		PRINT("Cant open file %s\n",fName);
		return(0);
	}

	sd_read_file(vers,8);
	sd_close_file();

	i |= vers[6] << 8;
	i |= vers[7];

	return(i);
}

// -----------------------------------------------------------------------------
// Description: Retrieve MS2 binary file version
//
// Details: Version information at Byte 0xFC and 0xFD
// 
// Called by: divers
//
// Return: 0 on error
// -----------------------------------------------------------------------------
uint16_t
get_ms2ver(char *fName)	{

	uint16_t i = 0;
	uint8_t vers[2];
	uint32_t seek = 0xFC;

	if(sd_open_file(fName))	{

		return(0);
	}

	sd_seek_file(&seek);
	sd_read_file(vers, 2);
	sd_close_file();

	i |= vers[0] << 8;
	i |= vers[1];

	return(i);
}


// -----------------------------------------------------------------------------
// Description: process the GFP 60133 update procedure
//
// Details:	Run a sequence to invoce the update
// 1) System Reset 
// 2) Invoce update request
// 3) Transfer binary file to target
// 4) System start
//
// Return: 0 on error
// -----------------------------------------------------------------------------
uint8_t
process_60133Update(void)	{

	//TODO
	//catch return values
	//Update LCD
	PRINT("System Reset\n");
	process_sysReset();
	PRINT("Boot init\n");
	process_bootInit();
	PRINT("Flash\n");
	process_bintransfer("016-gb2.bin", 512, GFP_BIN_MAGIC);
	return(0);
}

// -----------------------------------------------------------------------------
// Description: Process the 60113 initialization protocol 
//
// Details: After power up the 60113 wait for startup command. To identify
// the 60113 a bootInit command is send. 60113 response with UID/SW-Version 
// and device type. The following command start the 60113 software which
// need arround 400ms. At least a ping command will activate the 60113 which
// is responded like a normal ping.
//
// Sequence:
// MS2   ->C:0x1B R:0 H:0x036C D:0
// 60113 <-C:0x1B R:1 H:0x2120 D:8 D:0x47 0x43 0x66 0x63 0x01 0x27 0x00 0x10
// MS2   ->C:0x1B R:0 H:0x036C D:5 D:0x00 0x00 0x00 0x00 0x11
// MS2   ->C:0x18 R:0 H:0x036C D:0
//
// Note:  If a timeout occures in this synchrounous sequence the 
// initialization is failed completly
//
// Called by: 
//
// Return: 
// ----------------------------------------------------------------------------
uint8_t
init_60113(device_t *device)	{

	uint8_t cmd = 0;
	can_t msg;

	snd_bootInit();

	PRINT("60113 init\n");
	TCNT0 = count = 0;
	Flags |= (1 << TIMEOUT);

	while(Flags & (1 << TIMEOUT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_BOOTLD_CAN ) {	

				if((msg.id >> 16) & 1)	{

					device->hash = msg.id & 0xFFFF;
					device->uid |= (uint32_t)msg.data[0] << 24;
					device->uid |= (uint32_t)msg.data[1] << 16;
					device->uid |= (uint32_t)msg.data[2] << 8;
					device->uid |= (uint32_t)msg.data[3];
					device->sversion |= (uint16_t)msg.data[4] << 8;
					device->sversion |= (uint16_t)msg.data[5];
					device->type |= (uint8_t)msg.data[7];

					break;
				}
			}
		}
	}

	if(! cmd)	{

		PRINT("No GFP 60113!\n");
		return(1);
	}

	snd_bootStart();

	TCNT0 = count = 0;
	Flags |= (1 << DEVCALC);
	while(Flags & (1 << DEVCALC));		// wait 480ms for boot up target

	snd_ping();
	TCNT0 = count = cmd = 0;
	Flags |= (1 << TIMEOUT);

	while(Flags & (1 << TIMEOUT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_PING && ((msg.id >> 16) & 1))	{

				PRINT("Get Ping response\n");
				return(0);
			}
		}
	}

	return(1);
}

// -----------------------------------------------------------------------------
// Description: process the MS2 update procedure
//
// Details:	Run some sequences
//
// 1) Start up the MS2 to send it´s version requests
// 2) Dispatching all requests from MS2
// 3) Reboot MS2 and get it´s uid, hash and flash version 
// 4) If binary file was requested update the flash
//
// Return: 0 on error
// -----------------------------------------------------------------------------
uint8_t
process_MS2Update(device_t *device)	{

	uint8_t rt = 0;
	uint8_t flashOnce = 0;

	while(1)	{

		TCNT0 = count = 0;
		Flags |= (1 << FIRSTPNG);
		while(Flags & (1 << FIRSTPNG));		

		start_MS2(device->hash);
		rt = update_MS2();
		process_sysReset();

		if(rt == 0)	{
			PRINT("Update MS2 successfully done!!!!!!!!!!\n\n");
			break;
		}

		if(rt != REBOOT && rt != FLASHUPD)	{
			PRINT("Update get an error %d \n", rt);
			break;
		}

		init_MS2(device);

		if(rt == FLASHUPD)	{

			if(! flashOnce)
				if(process_flashUpd())
					break;

			flashOnce = 1;
		}
	}

	return(rt);
}

// -----------------------------------------------------------------------------
// Description: Process the MS2 initialization protocol 
//
// Details: After power up the MS2 send a 0x1B command sequence. The MS2 then
// start the 0x18 ( ping ) commands every 30sec.
// Send a 0x18 ping command to retreive MS2 UID and loaded software version
//
// Sequence:
// MS2 ->C:0x1B R:0 H:0x036C D:0
// MS2 ->C:0x1B R:0 H:0x036C D:5 D:0x00 0x00 0x00 0x00 0x11
// MS2 ->C:0x18 R:0 H:0x036C D:0
//
// Note:  If a timeout occures in this synchrounous sequence the 
// initialization is failed completly
//
// Called by: main()
//
// Return: 0 on success otherwise 1
// ----------------------------------------------------------------------------
uint8_t
init_MS2(device_t *device)	{

	uint8_t cmd = 0;
	can_t msg;

	// Wait a little time until MS2 send init sequence is complete
	TCNT0 = count = 0;
	Flags |= (1 << DEVBOOT);

	while(Flags & (1 << DEVBOOT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_BOOTLD_CAN)	{

				// Get additional message on MS2 Version > 1.83
				// MS2 wait 400ms after sending start msg 
				cmd = 0;
				TCNT0 = count = 0;
				Flags |= (1 << DEVCALC);

				while(Flags & (1 << DEVCALC))	{

					if(read_rx_buffer(&msg))	{

						cmd = (msg.id >> 17) & 0xFF;

						if(cmd == CMD_BOOTLD_CAN ) {	// Ignore 60113 start msg
							snd_ping();
							break;
						}
					}
				}

				if(!cmd) {		// MS2 Version <= 1.8.3 or deleted gb2 file
					PRINT("VERSION <= 1.83 protocoll detect\n");
					snd_ping();
				}

				TCNT0 = count = 0;
				Flags |= (1 << TIMEOUT);

				while(Flags & (1 << TIMEOUT))	{

					if(read_rx_buffer(&msg))	{

						cmd = (msg.id >> 17) & 0xFF;

						if(cmd == CMD_PING ) {	

							if((msg.id >> 16) & 1)	{

								device->hash = msg.id & 0xFFFF;
								device->uid |= (uint32_t)msg.data[0] << 24;
								device->uid |= (uint32_t)msg.data[1] << 16;
								device->uid |= (uint32_t)msg.data[2] << 8;
								device->uid |= (uint32_t)msg.data[3];
								device->sversion |= (uint16_t)msg.data[4] << 8;
								device->sversion |= (uint16_t)msg.data[5];
								device->type |= (uint8_t)msg.data[7];

								return(0);
							}
						}
					}
				}
			}
		}
	}

	PRINT("No MS2 !\n");
	return(1);
}

// -----------------------------------------------------------------------------
// Description: Start MS2 update procedure
//
// Details:	Response a ping request with requesters hash plus magic device
// ID ( possible of CS2 ). This invoce a request seqence of 0x20 ( Configdata )
// to the initiating device.
//
//
// Return: 0 on error
// -----------------------------------------------------------------------------
static void
start_MS2(uint16_t hash)	{

	uint8_t cmd = 0;
	can_t msg;

	TCNT0 = count = 0;
	Flags |= (1 << TIMEOUT);

	while(Flags & (1 << TIMEOUT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_PING)	{

				// free rx_buffer
				clear_rx_buffer();

				// This initiate a data exchange cmd 0x20
				// and is the start procedure of an MS2 update
				resp_ping(hash);	 
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Description: MS2 config data request dispatcher
//
// Details:	 Call functions which match to requested filename. If the version 
// of the requested file is different, MS2 request to send data.
//
// Return: state to indicate next processing
// -----------------------------------------------------------------------------
static uint8_t
update_MS2(void)	{

	uint8_t cmd = 0, i = 0, callerID = 0, rt = ETIMED, vercnt = 0;;
	uint8_t cfgName[9];
	can_t msg;

	TCNT0 = count = 0;
	Flags |= (1 << DEVBOOT);

	while(Flags & (1 << DEVBOOT))	{

		if(read_rx_buffer(&msg))	{

			cmd = (msg.id >> 17) & 0xFF;

			if(cmd == CMD_CFG_REQUEST)	{

				cfgName[0] = msg.data[0];
				cfgName[1] = msg.data[1];
				cfgName[2] = msg.data[2];
				cfgName[3] = msg.data[3];
				cfgName[4] = msg.data[4];
				cfgName[5] = msg.data[5];
				cfgName[6] = msg.data[6];
				cfgName[7] = msg.data[7];
				cfgName[8] = 0;

				PRINT("MS2 request: %s -->\n ",cfgName);

				// Multiple request command
				// Used by invoke transfer data
				// to requester
				if(callerID)	{

					PRINT("invoce transferfile\n");
					resp_cfg_request(cfgName); // Respone requested block #0

					rt = (*caller[callerID].function)(caller[callerID].fName);

					if(caller[callerID].flashUpd)	{
						PRINT("Need FLASH update\n");
						return(FLASHUPD);
					}else {
						PRINT("Another request?? rt=%d\n",rt);
						callerID = 0;
					}

				} else {

					for(i = 0; i < FKTCOUNT; i++)	{

						if(strcmp(caller[i].fktName, (char *)cfgName) == 0 )	{

							if(caller[i].reqCount)	{

								callerID = i;
								break;

							} else {

								PRINT("invoce version control\n");
								resp_cfg_request(cfgName);
								rt = (*caller[i].function)(caller[i].fName);
								vercnt++;
								break;
							}
						}
					}
				}
			}
		}
		//short up if nothing to do
		if((vercnt == VERCOUNT) && (callerID == 0)){
			PRINT("Nothing to do...\n");
			break;
		}
	}

	PRINT("End dispatcher rt=%d\n",rt);
	return(rt);
}
