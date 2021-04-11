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
#include "Flash.h"
// this actually generates less code than a function
static bool wait_ready()            
{
	uint32_t to=FLASH_TIMEOUT_MS;
	while (NVMCTRL->STATUS.bit.READY == 0)
	{
		if (--to == 0)
		{
			ERROR("Flash time out");
			return false;
		}
		delay_ms(1);
	}
	return true;
}


static bool wait_done()            
{
	uint32_t to=FLASH_TIMEOUT_MS;
	while (NVMCTRL->INTFLAG.bit.DONE == 0)
	{
		if (--to == 0)
		{
			ERROR("Flash time out");
			return false;
		}
		delay_ms(1);
	}
	return true;
}


bool flashInit(void){
	if (NVMCTRL->PARAM.bit.PSZ != 6)
	{
		ERROR("FLASH PAGE SIZE is not 512 bytes");
		return false;
	}
	MCLK->AHBMASK.reg |= MCLK_AHBMASK_NVMCTRL;
	return true;
}


static void erase(const volatile void *flash_ptr)
{
	if(!wait_ready())
	{
		ERROR("not ready");
		return; 
	}
	bool isrState=InterruptDisable();
	NVMCTRL->INTFLAG.bit.DONE=1;
	NVMCTRL->ADDR.reg = ((uint32_t)flash_ptr);
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB;
	if(!wait_done())
	{
		ERROR("not done");
	}
	InterruptEnable(isrState);
}

static void eraseUserPage(void)
{
	
	
	bool isrState=InterruptDisable();
	wait_ready();
	NVMCTRL->INTFLAG.bit.DONE=1;
	NVMCTRL->ADDR.reg = ((uint32_t)NVMCTRL_USER);
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EP;
	wait_done();
	InterruptEnable(isrState);
}

bool flashErase(const volatile void *flash_ptr, uint32_t size)
{
	const uint8_t *ptr = (const uint8_t *)flash_ptr;
	uint8_t *ptr2 = (uint8_t *)flash_ptr;
	uint32_t n;
	bool erased=true;
	n=0;
	while(n<size)
	{
		if (*ptr2!=0xFF)
		{
			erased=false;
			break;
		}
		ptr2++;
		n++;
	}
	if (erased)
	{
		return true;
	}

	while (size > FLASH_BLOCK_SIZE) {
		erase(ptr);
		ptr += FLASH_BLOCK_SIZE;
		size -= FLASH_BLOCK_SIZE;
	}
	if (size>0)
	{
		erase(ptr);
	}


	//  THIS CODE DOES NOT WORK READING CACHE OR SOMETHING
	//		//flush the Cortex cache
	CMCC->CTRL.bit.CEN=0;
	while(CMCC->SR.bit.CSTS==1) {}
	CMCC->MAINT0.bit.INVALL=1;
	CMCC->CTRL.bit.CEN=1;

	erased=true;
	n=0;
	ptr2 = (uint8_t *)flash_ptr;
	while(n<size)
	{
		uint8_t d,j;
		j=0;
		flashRead(ptr2,&d,1);
		while(d != 0xFF && j<10)
		{
			j++;
			flashRead(ptr2,&d,1);
		}
		if (d != 0xFF)
		{
			erased=false;
			break;
		}
		ptr2++;
		n++;
	}
	if (!erased)
	{
		ERROR("could not erase flash at 0x%X, %d bytes", (uint32_t)flash_ptr,size);
	}
	return true; //TODO should verify the erase
}

static inline uint32_t read_unaligned_uint32(const void *data)
{
	union {
		uint32_t u32;
		uint8_t u8[4];
	} res;
	const uint8_t *d = (const uint8_t *)data;
	res.u8[0] = d[0];
	res.u8[1] = d[1];
	res.u8[2] = d[2];
	res.u8[3] = d[3];
	return res.u32;
}

static void flashWriteWords(uint32_t *flash_ptr,const uint32_t *data, uint32_t numWords)
{

	uint32_t to;

	if (((uint32_t)flash_ptr & 0x0F) !=0)
	{
		ERROR("Flash Write Words must be on 16 byte boundary");
		return;
	}
	bool isrState=InterruptDisable();

	// The cache in Rev A isn't reliable when reading and writing to the NVM.
	NVMCTRL->CTRLA.bit.CACHEDIS0 = true;
	NVMCTRL->CTRLA.bit.CACHEDIS1 = true;

	// Set manual page write
	NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN;

	//clear the Page buffer
	wait_ready();
	NVMCTRL->INTFLAG.bit.DONE=1;
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;
	while (NVMCTRL->INTFLAG.bit.DONE == 0) { }
	wait_ready();

	uint32_t *src_addr=(uint32_t *)data;
	uint32_t *dst_addr=(uint32_t *)flash_ptr;
	while (numWords>0)
	{
		uint32_t j;
		uint32_t addr=(uint32_t)dst_addr;

		if (!wait_ready())
		{
			ERROR("Flash timeout ");
			goto flash_end;
		}
		//write a quad word
		for(j=0; j<4; j++)
		{
			if (numWords>0)
			{
				*dst_addr = *src_addr;
				src_addr++;
				numWords--;
			}else
			{
				*dst_addr = 0xFFFFFFFF;
			}
			dst_addr++;

		}
		NVMCTRL->INTFLAG.bit.DONE=1;
		NVMCTRL->ADDR.reg = ((uint32_t)addr);
		NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WQW;
		to=FLASH_TIMEOUT_MS;
		while (!NVMCTRL->INTFLAG.bit.DONE)
		{
			if (--to == 0)
			{
				ERROR("Flash write failed, timeout");
				goto flash_end;
			}
			delay_ms(1);
		}
		if (!wait_ready())
		{
			ERROR("Flash timeout ");
			goto flash_end;
		}
	}

	flash_end:
	// The cache in Rev A isn't reliable when reading and writing to the NVM.
	NVMCTRL->CTRLA.bit.CACHEDIS0 = false;
	NVMCTRL->CTRLA.bit.CACHEDIS1 = false;
	InterruptEnable(isrState);
}

void flashWriteUser(const volatile void *flash_ptr,const void *data, uint32_t size)
{
	__attribute__((__aligned__(4))) uint8_t buffer[FLASH_USER_PAGE_SIZE];

	if (((uint32_t)flash_ptr)<(NVMCTRL_USER+FLASH_USER_RESERVED)
			|| ((uint32_t)flash_ptr + size)>(NVMCTRL_USER+FLASH_USER_PAGE_SIZE))
	{
		ERROR("data not in valid USER page area");
		return;
	}
	ASSERT(FLASH_USER_PAGE_SIZE==FLASH_PAGE_SIZE);

	flashRead((void *)NVMCTRL_USER,buffer,sizeof(buffer));

	//copy the new data into the buffer
	memcpy((void *)(&buffer[(uint32_t)flash_ptr-NVMCTRL_USER]),data,size);

	eraseUserPage();

	flashWriteWords((uint32_t *)NVMCTRL_USER,(uint32_t *)buffer,FLASH_USER_PAGE_SIZE/4); //4 bytes in a word
}

#if 1
//void flashWritePage(const volatile void *flash_ptr, const void *data, uint32_t size);
uint32_t flashWrite(const volatile void *flash_ptr,const void *data, uint32_t size)
{
	volatile uint32_t *ptrPage;
	uint8_t *destPtr;
	uint8_t *srcPtr;
	uint32_t bytesInBlock;
	__attribute__((__aligned__(4))) uint8_t buffer[FLASH_BLOCK_SIZE];
	uint32_t offset;
	uint32_t n=size;

	destPtr=(uint8_t *)flash_ptr;
	srcPtr=(uint8_t *)data;

	LOG("flash write called addr 0x%X",destPtr);
	while(size>0)
	{
		uint32_t i,j;
		//volatile uint32_t x;

		//calculate the maximum number of bytes we can write in page
		offset=((uint32_t)destPtr)%(FLASH_BLOCK_SIZE); //offset into page
		bytesInBlock=FLASH_BLOCK_SIZE-offset; //this is how many bytes we need to overwrite in this page

		//LOG("offset %d, bytesInBlock %d size %d", offset, bytesInBlock,size);
		//get pointer to start of page
		ptrPage=(uint32_t *) ((((uint32_t)destPtr)/(FLASH_BLOCK_SIZE)) * FLASH_BLOCK_SIZE);

		//LOG("pointer to page %d(0x%08x) %d",(uint32_t)ptrPage,(uint32_t)ptrPage,destPtr);

		//fill page buffer with data from flash
		memcpy(buffer,(void *)ptrPage,FLASH_BLOCK_SIZE);

		//now fill buffer with new data that needs changing
		i=bytesInBlock;
		if (size<i)
		{
			i=size;
		}
		//LOG("changing %d bytes",i);
		memcpy(&buffer[offset],srcPtr,i);

		//erase page
		flashErase(ptrPage,FLASH_BLOCK_SIZE);
		//write new data to flash
		flashWritePage((const volatile void *)ptrPage,(const void *)buffer,FLASH_BLOCK_SIZE);


		//delay_ms(10);

		//flush the Cortex cache
		CMCC->CTRL.bit.CEN=0;
		//while(CMCC->SR.bit.CSTS==0) {}
		CMCC->MAINT0.bit.INVALL=1;
		CMCC->CTRL.bit.CEN=1;

		//		x=-1;
		//		while(x==-1)
		//		{
		//		    flashRead((const volatile void *)ptrPage, (void *)&x, sizeof(x));
		//		}
		//		
		volatile uint32_t *ptr=(uint32_t *)buffer;
		for (j=0; j<FLASH_BLOCK_SIZE/4; j++)
		{
			bool good=false;
			uint32_t ii;
			for (ii=0; ii<2; ii++) //seems we have to read flash multiple times to get good value
			{
				volatile uint32_t x;

				NVMCTRL->CTRLA.bit.CACHEDIS0 = true;
				NVMCTRL->CTRLA.bit.CACHEDIS1 = true;

				__ASM volatile (" dsb":::"memory");
				__ASM volatile (" isb":::"memory");
				x= *(uint32_t *)0x004; //read address zero to flush flash cache
				__ASM volatile (" dsb":::"memory");
				__ASM volatile (" isb":::"memory");
				if (*ptrPage == *ptr)
				{
					good=true;
				}

				NVMCTRL->CTRLA.bit.CACHEDIS0 = false;
				NVMCTRL->CTRLA.bit.CACHEDIS1 = false;

			}
			if (good == false)
			{

				ERROR("write failed on word %d %x %x",j,*ptrPage, *ptr);

			}
			ptrPage++;
			ptr++;
		}


		size=size-i; //decrease number of bytes to write
		srcPtr+=i; //increase pointer to next bytes to read
		destPtr+=i; //increment destination pointer
	}
	return n;

}

uint32_t flashWritePage(const volatile void *flash_ptr, const void *data, uint32_t size)
{
	//uint32_t n=size;
	// Calculate data boundaries
	bool isrState;
	size = (size + 3) / 4; //convert bytes to words with rounding

	volatile uint32_t *dst_addr = (volatile uint32_t *)flash_ptr;
	const uint8_t *src_addr = (uint8_t *)data;
	
	volatile uint32_t write_addr; 

	if (0 != ((uint32_t)flash_ptr)%(FLASH_PAGE_SIZE))
	{
		ERROR("Flash page write must be on boundry");
		return 0;
	}

	while (NVMCTRL->STATUS.bit.READY == 0) { }

	// Disable automatic page write
	NVMCTRL->CTRLA.bit.WMODE = 0x00;

	LOG("Writting Page 0x%x",(uint32_t)flash_ptr);
	// Do writes in pages
	while (size)
	{
		//bool isrState=InterruptDisable();

		// The cache in Rev A isn't reliable when reading and writing to the NVM.
		//	    NVMCTRL->CTRLA.bit.CACHEDIS0 = true;
		//	    NVMCTRL->CTRLA.bit.CACHEDIS1 = true;

		isrState=InterruptDisable();
		NVMCTRL->INTFLAG.bit.DONE=1;
		// Execute "PBC" Page Buffer Clear
		NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;
		wait_ready();
		InterruptEnable(isrState);

		// Fill page buffer
		write_addr=(uint32_t)dst_addr;
		uint32_t i;
		for (i=0; i<(FLASH_PAGE_SIZE/4) && size; i++) //we write 4 bytes at a time
		{
			*dst_addr = read_unaligned_uint32(src_addr);
			src_addr += 4;
			dst_addr++;
			size--; //size is set to number of 32bit words in first line above
		}
		wait_ready();

		isrState=InterruptDisable();
		NVMCTRL->INTFLAG.bit.DONE=1;
		NVMCTRL->ADDR.reg=write_addr;
		// Execute "WP" Write Page
		NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WP;	
		wait_done();
		InterruptEnable(isrState);

		// The cache in Rev A isn't reliable when reading and writing to the NVM.
		//		NVMCTRL->CTRLA.bit.CACHEDIS0 = false;
		//		NVMCTRL->CTRLA.bit.CACHEDIS1 = false;
		//		bool isrState=InterruptDisable();
		//		
		//flush the Cortex cache
		CMCC->CTRL.bit.CEN=0;
		//while(CMCC->SR.bit.CSTS==0) {}
		CMCC->MAINT0.bit.INVALL=1;
		CMCC->CTRL.bit.CEN=1;
		//InterruptEnable(isrState);

		wait_ready();
		wait_done();
	}

	return size;
}

#endif
