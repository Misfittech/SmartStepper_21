/**********************************************************************
 *      Author: tstern
 *
 *	Misfit Tech invests time and resources providing this open source code,
 *	please support Misfit Tech and open-source hardware by purchasing
 *	products from Misfit Tech, www.misifittech.net!
 *
 *	Written by Trampas Stern  for Misfit Tech.
 *	BSD license, check license.txt for more information
 *	All text above, must be included in any redistribution
 *********************************************************************/
#ifndef __FLASH__H__
#define __FLASH__H__

#include "board.h"

#define FLASH_TIMEOUT_MS  (1000)

//pages per block defined 25.62 "memory organization"
#define FLASH_BLOCK_SIZE (FLASH_PAGE_SIZE*16) //defined in the datasheet as 16 pages per block

#define FLASH_ERASE_VALUE (0xFF) //value of flash after an erase

#define FLASH_USER_RESERVED (32) //first 32 bytes of user page is reserved

#define FLASH_ALLOCATE(name, size) \
	__attribute__((__aligned__(FLASH_BLOCK_SIZE))) \
   const uint8_t name[(size+(FLASH_BLOCK_SIZE-1))/FLASH_BLOCK_SIZE*FLASH_BLOCK_SIZE] = { };

bool flashInit(void); //this checks that our assumptions are true

void flashWriteUser(const volatile void *flash_ptr,const void *data, uint32_t size);

bool flashErase(const volatile void *flash_ptr, uint32_t size);
uint32_t flashWrite(const volatile void *flash_ptr,const void *data,uint32_t size);
uint32_t flashWritePage(const volatile void *flash_ptr, const void *data, uint32_t size);

//you can read by dereferencing pointer but we will add a read
static inline uint32_t flashRead(const volatile void *flash_ptr, void *data, uint32_t size)
{

	if (((uint32_t)flash_ptr+size)>(FLASH_SIZE+FLASH_ADDR))
	{
		ERROR("bad flash address 0x%X",(uint32_t)flash_ptr);
		memset(data,0,size);
		return 0;
	}
	bool isrState=InterruptDisable();
	// The cache in Rev A isn't reliable when reading and writing to the NVM.
    NVMCTRL->CTRLA.bit.CACHEDIS0 = true;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = true;
  memcpy(data, (const void *)flash_ptr, size);
  // The cache in Rev A isn't reliable when reading and writing to the NVM.
    NVMCTRL->CTRLA.bit.CACHEDIS0 = false;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = false;
   InterruptEnable(isrState);
   return size;
}




#endif //__FLASH__H__
