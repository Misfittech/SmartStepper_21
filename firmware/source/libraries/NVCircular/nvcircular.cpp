/*
 * nvsettings.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: tstern
 */

#include "nvcircular.h"

#include "./libraries/syslog/syslog.h"
#include "./drivers/systick/systick.h"
#include "./drivers/flash/flash.h"
/*
 *
 *
 * The data is stored in flash as a structure:
 *
 *  uint16_t size
 *  uint32_t seconds
 *  uint8_t data[N] //where N is any length
 *  uint8_t crc;  //includes header (size and seconds) and data...
 *
 *  TODO we can speed this up by caching the sector starts in
 *  a ram table.
 *
 */



typedef struct __attribute__((packed)) {
	uint32_t size; //size of the data N, does not include header or CRC
	uint32_t seconds;
	uint32_t writeIndex; //increments on each write.
} header_t;



const uint8_t CRC7_POLY = 0x91;

static uint8_t getCRCForByte(uint8_t val, uint8_t crc)
{
	uint8_t j;

	crc ^= val;
	for (j = 0; j < 8; j++)
	{
		if (crc & 1)
			crc ^= CRC7_POLY;
		crc >>= 1;
	}

	return crc;
}


NVCircular::NVCircular(void)
{
	_sectorSize=0;

	_lastWriteAddress=(uint32_t)-1;
	_cache.seconds=(uint32_t)-1;
	_cache.address=(uint32_t)-1;
	_writeIndex=0;
}


void NVCircular::erase(void)
{
	//uint32_t address;

	flashErase((void *)_startAddress,_endAddress-_startAddress);
	_lastWriteAddress=(uint32_t)-1;
	_cache.seconds=(uint32_t)-1;
	_cache.address=(uint32_t)-1;
	_writeIndex=0;
}


//TODO this assumes address starts on sector boundary, need to verify this

bool NVCircular::begin(uint32_t startAddress, uint32_t endAddress, uint32_t sectorSize)
{
	uint32_t n;
	_startAddress=startAddress;
	_endAddress=endAddress;
	_sectorSize=sectorSize;
	_writeIndex=0;

	n=((_endAddress+1)-_startAddress)/_sectorSize;

	if ((_startAddress + (n*_sectorSize)) !=  (_endAddress +1) || n<2)
	{
		ERROR("Start and end addresses must be at least two full sectors");
		_sectorSize=0;
		return false;
	}
	//find the most recent sector to get the correct writeindex
	findMostRecentSector();

	return true;
}

uint8_t NVCircular::calculateCRC(uint8_t *ptrData, size_t nBytes, uint8_t crc)
{
	uint32_t i;

	i=0;
	while(nBytes)
	{
		crc=getCRCForByte(ptrData[i],crc);
		//LOG("%d 0x%x",i,crc);
		i++;
		nBytes--;
	}
	return crc;
}

uint8_t NVCircular::getCRC(uint32_t address, size_t nBytes)
{
	uint8_t crc;

	uint8_t data[100];

	crc=0;

	while(nBytes)
	{

		uint32_t i;
		flashRead((void *)address,data, sizeof(data));
		for (i=0; i<100; i++)
		{
			crc=getCRCForByte(data[i],crc);
			//LOG("%d 0x%x",i,crc);
			nBytes--;
			if (nBytes==0)
			{
				break;
			}
		}
		address+=100;

	}
	return crc;
}

bool NVCircular::checkData(uint32_t address)
{

	uint16_t sec;
	uint32_t crcAddress;
	uint8_t crc, crcFlash;
	header_t header;



	if (address>_endAddress)
	{
		//this is an invalid address
		return false;
	}



	sec=address/_sectorSize;

	flashRead((void *)address,(uint8_t *)&header, sizeof(header));

	crcAddress = address+ header.size+ sizeof(header); //increment to get address of CRC

	if (crcAddress>_endAddress)
	{
		//this is an invalid address
		return false;
	}
	if (crcAddress/_sectorSize != sec)
	{
		//we crossed sector boundry which is not allowed
		return false;
	}

	flashRead((void *)crcAddress,&crcFlash, sizeof(crcFlash));

	//include size in CRC

	crc=getCRC(address, header.size+sizeof(header));




	if (crc==crcFlash && header.seconds!=0)
	{
		return true;
	}
	ERROR("CRC check failed");
	return false;
}


uint32_t NVCircular::findMostRecentSector(void)
{
	uint32_t address;
	uint32_t lastValidAddress=_startAddress;
	header_t header;
	uint32_t seconds=0;
	_writeIndex=0;

	address=_startAddress;
	
	while((address+sizeof(header_t))<_endAddress)
	{
	    LOG("Reading 0x%x",address);
		flashRead((void *)address,(uint8_t *)&header, sizeof(header));
		if (header.size != (uint32_t) (-1))
		{
			//check that data is valid
			if (checkData(address))
			{
				if (seconds< header.seconds || (_writeIndex < header.writeIndex && seconds == header.seconds))
				{
					seconds=header.seconds;
					_writeIndex=header.writeIndex;
					lastValidAddress=address;
					LOG("Found packet 0x%" PRIX32 , address);
				}
			}else
			{
				LOG("check failed packet 0x%" PRIX32 , address);
			}
		}else
		{
			//LOG("packet 0x%" PRIX32 ",%" PRIu32, address,(uint32_t)header.size);
		}
		address+=_sectorSize;
	}
	return lastValidAddress;
}


bool NVCircular::getLastValidPacket(uint32_t *ptrAddress)
{
	uint32_t address;

	uint32_t lastValidAddress=(uint32_t)-1;
	header_t header;


	//lets start at the start address and read the data until we find
	// the one where the next block size is 0xFFFF


	address=findMostRecentSector();

	if (false == checkData(address))
	{
		WARNING("FLash is messed up clearing");
		//flash is empty... or corrupt
		flashErase((void *)_startAddress,_sectorSize);
		*ptrAddress=_startAddress;
		return false;
	}
	while((address+sizeof(header))<_endAddress)
	{
		flashRead((void *)address,(uint8_t *)&header, sizeof(header));
		if (header.size == (uint32_t) (-1))
		{
		    LOG("failed read packet 0x%" PRIX32 , address);
			if (lastValidAddress != (uint32_t)-1)

			{
				*ptrAddress=lastValidAddress;
				return true;
			}
			address+=2;

		}else if (header.size == 0)
		{
			address+=2;
		} else
		{
			//check that data is valid
			if (checkData(address))
			{
				lastValidAddress=address;
				LOG("Found packet 0x%" PRIX32 , address);

				address = address+ header.size+ sizeof(header)+1; //increment to get address of next packet, add one for CRC

				if (address & 0x01)
				{
					address++; // size is always on uint16_t boundry (even byte)
				}
			} else
			{
				//increment address
				address+=2;
			}
		}

	}
	//packet not found
	return false;
}


size_t NVCircular::read(uint8_t *ptrData, size_t maxBytes)
{
	uint32_t address;

	if (getLastValidPacket(&address))
	{
		header_t header;
		flashRead((void *)address,(uint8_t *)&header, sizeof(header));
		if (maxBytes>=header.size)
		{
			flashRead((void *)(address+sizeof(header)),ptrData, header.size);
		} else
		{
			ERROR("The maxBytes is not enough to hold the data %" PRIu32,header.size);
			header.size=0;
		}
		return header.size;


	}
	return 0;
}

size_t NVCircular::write(uint8_t *ptrData, size_t nBytes)
{
	uint32_t address;
	uint32_t nextPacketAddress;
	uint32_t nextSectorAddress;
	uint32_t sector;

	header_t header;
	bool ret;

	//uint32_t packetSize;

	//packetSize = nBytes+sizeof(header)+1;  //size in flash is packet size plus 2 bytes for size, one for CRC


	if (_lastWriteAddress != (uint32_t)-1)
	{
		address=_lastWriteAddress;
		ret=true;
	}else
	{
		ret=getLastValidPacket(&address);
	}
	if (ret)
	{

		flashRead((void *)address,(uint8_t *)&header, sizeof(header));

		nextPacketAddress=address+header.size+sizeof(header)+1; //next packet packet size plus size bytes plus crc byte

		if (nextPacketAddress & 0x01)
		{
			nextPacketAddress++;
		}
		LOG("found packet 0x%X", address);
		LOG("next write is at 0x%X",nextPacketAddress);
		//need to check that our data does not cross flash boundry
		sector=address/_sectorSize;

		if (sector != ((nextPacketAddress)/_sectorSize))
		{

			//next packet crosses sector boundary...

			// find the next sector address;
			nextSectorAddress=(sector+1)*_sectorSize;
			if (nextSectorAddress>=_endAddress)
			{
				nextSectorAddress=_startAddress;
			}

			//erase this sector
			flashErase((void*)nextSectorAddress,_sectorSize);

			//determine how many sectors the data takes
			sector=nBytes/_sectorSize;
			if (sector>0)
			{
				ERROR("The circular buffer can not handle data larger than sector size at moment");
				return 0;
			}
			//we need to fill the last n bytes of sector with zeros
			while (nextPacketAddress != nextSectorAddress)
			{
				uint16_t data=0;
				flashWrite((void *)nextPacketAddress, (uint8_t *)&data, sizeof(data));
				nextPacketAddress+=2;
				if (nextPacketAddress>=_endAddress)
				{
					nextPacketAddress=_startAddress;
				}

			}
		}
	} else
	{
		//no valid data found
		nextPacketAddress=_startAddress;
		flashErase((void*)nextPacketAddress,_sectorSize);
	}
	//see if we have room for data
	if (1)//ptrFlash->verifyErased(nextPacketAddress, packetSize))
	{
		uint8_t crc;
		uint32_t startPacketAddress=nextPacketAddress;

		header.size=nBytes;
		if (_lastWriteSeconds == factorySeconds())
		{
			WARNING("Two packets with same time stamp");
		}

		_lastWriteSeconds=porSeconds();
		_writeIndex++;
		header.writeIndex=_writeIndex;
		header.seconds=_lastWriteSeconds;

		_lastWriteAddress=nextPacketAddress;
		//LOG("Writting Packet to 0x%" PRIX32, nextPacketAddress);
		//write header to flash
		flashWrite((void *)nextPacketAddress, (uint8_t *)&header, sizeof(header));
		//write data to flash
		flashWrite((void *)(nextPacketAddress+sizeof(header)), ptrData, nBytes);

		crc = calculateCRC((uint8_t *)&header, sizeof(header),0);
		crc= calculateCRC(ptrData, nBytes, crc);

//		uint8_t crc2= getCRC(nextPacketAddress,nBytes+sizeof(header));
//		if (crc != crc2)
//		{
//			ERROR("CRC does not match");
//		}
		nextPacketAddress=nextPacketAddress+nBytes+sizeof(header);

		flashWrite((void *)nextPacketAddress, &crc, sizeof(crc));

		if (checkData(startPacketAddress) == false)
		{
			ERROR("Write to address 0x%" PRIX32 " failed",startPacketAddress);
		}
		return nBytes;
	}
	ERROR("Data was not erased, erasing flash and starting over");
	erase();
	return 0;

}


uint32_t NVCircular::findSectorJustBefore(uint32_t seconds)
{
	uint32_t address;
	uint32_t lastValidAddress=_startAddress;
	header_t header;
	uint32_t testSeconds=0;

	address=_startAddress;

	while(address<_endAddress)
	{
		flashRead((void *)address,(uint8_t *)&header, sizeof(header));
		if (header.size != (uint32_t) (-1))
		{
			//check that data is valid
			if (checkData(address))
			{
				if (header.seconds<=seconds && header.seconds>testSeconds)
				{
					testSeconds=header.seconds;
					lastValidAddress=address;
					//LOG("Found packet 0x%" PRIX32 , address);
					address = address+ header.size+ sizeof(header)+1; //increment to get address of next packet
					if (address & 0x01)
					{
						address++; // size is always on uint16_t boundary (even byte)
					}
				}
			}
		}
		address+=_sectorSize;
	}
	return lastValidAddress;
}


uint32_t NVCircular::numberPacketsNewerThan(uint32_t seconds)
{
	uint32_t address;
	header_t header;
	uint32_t nBytesPerPacket;
	uint32_t nextSectorAddress;

	uint32_t nPackets=0;
	bool done=false;

	//lets start at the start address and read the data until we find
	// the one where the next block size is 0xFFFF

	address=findSectorJustBefore(seconds);

	nextSectorAddress= ((address/_sectorSize)+1)*_sectorSize;

	//Read count number of packets until next full sector boundary
	while (address<nextSectorAddress)
	{
		flashRead((void*)address,(uint8_t *)&header, sizeof(header));
		if (header.size == (uint16_t)-1)
		{
			//end of data
			return nPackets;
		}
		if (header.size == 0)
		{
			address=nextSectorAddress;
			break;
		}

		if (checkData(address))
		{
			//header is valid
			if (header.seconds >seconds)
			{
				nPackets++;
			}
			address+=header.size+sizeof(header)+1;
			if (address & 0x01)
			{
				address++;
			}
		}else
		{
			address+=2;
		}
	}


	uint32_t startSector=(address/_sectorSize) * _sectorSize;
	bool looped=false;
	bool differentSector=false;

	while(!done)
	{
		uint32_t sect;
		sect = (nextSectorAddress/_sectorSize) * _sectorSize;

		if (startSector != sect) //if we are in different sector
		{
			differentSector=true;
			if (looped)
			{
				//we have looped around flash once and not found our data.
				ERROR("Most like time or flash is corrupt");
				erase();

			}
		}

		if (differentSector)
		{
			if (sect==startSector)
			{
				//we have looped back around to starting sector
				looped=true;
			}
		}
		//read the header for the next sector
		nextSectorAddress=address+_sectorSize;
		if (nextSectorAddress>_endAddress)
		{
			nextSectorAddress=_startAddress;
			LOG("Read through flash and starting over");
		}

		flashRead((void *)nextSectorAddress,(uint8_t *)&header, sizeof(header));
		if (header.size!=0 && header.size != (uint16_t)-1)
		{

			//header is valid
			if (header.seconds >seconds)
			{

				//assume previous sector is full of same size objects
				nBytesPerPacket=(header.size+sizeof(header)+1);
				if (nBytesPerPacket & 0x01) //we always start on even address
				{
					nBytesPerPacket++;
				}
				nPackets+=_sectorSize/nBytesPerPacket;
			}
			address=nextSectorAddress;

		}else
		{
			//lets scan the address;
			flashRead((void *)address,(uint8_t *)&header, sizeof(header));
			if (header.size!=0 && header.size != (uint16_t)-1)
			{
				//header is valid
				nPackets++;
				address += header.size + sizeof(header) + 1;
				if (address & 0x01)
				{
					address++;
				}
			} else
			{

				return nPackets;
			}
		}
	}

	return 0;

}

uint32_t NVCircular::readNextPacket(uint8_t *ptrData, size_t maxBytes, uint32_t seconds)
{
	uint32_t address;
	header_t header;
	bool done=false;

	//lets start at the start address and read the data until we find
	// the one where the next block size is 0xFFFF

	address=(uint32_t)-1;
	if (_cache.seconds==seconds)
	{
		address=_cache.address; //start at last address to make a bit faster
	}
	if (address  == (uint32_t)-1)
	{
		address=findSectorJustBefore(seconds);
	}

	while(!done)
	{
		flashRead((void*)address,(uint8_t *)&header, sizeof(header));
		if (header.size == 0xFFFF)
		{
			//this is the end of the data...
			break;
		}else if (header.size == 0 && header.seconds==0)
		{
			//the remaining data in this sector is of no use
			// jump to next sector;
			uint32_t sector;
			//LOG("sector end 0x%" PRIX32 , address);
			sector=address/_sectorSize;
			sector++;
			address=sector*_sectorSize;
			//LOG("sector 0x%" PRIX32 , address);
		} else
		{

			//check that data is valid
			if (checkData(address))
			{
				if (header.seconds>seconds)
				{
					//Guess next sample time and address
					//LOG("data @ 0x%" PRIX32 , address);
					_cache.seconds=header.seconds; //+1;
					_cache.address=address; // + header.size+ sizeof(header)+1;
					address += sizeof(header);
					if (header.size<=maxBytes)
					{

						flashRead((void *)address,(uint8_t *)ptrData, header.size);
						return header.seconds;
					}else
					{
						ERROR("PAcket is bigger than buffer");
						return 0;
					}
				}else
				{
					address = address+ header.size+ sizeof(header)+1; //increment to get address of next packet
					if (address & 0x01)
					{
						address++; // size is always on uint16_t boundry (even byte)
					}
				}
			} else
			{
				//increment address
				LOG("Bad Packet found %" PRIX32, address);
				address+=2;
			}
		}

	}
	//packet not found
	return 0;
}

uint32_t NVCircular::readLastPacket(uint8_t *ptrData, size_t maxBytes)
{
	uint32_t address;
	bool ret;

	if (_lastWriteAddress != (uint32_t)-1)
	{
		address=_lastWriteAddress;
		ret=true;
	}else
	{
		ret=getLastValidPacket(&address);
	}
	if (ret)
	{
		header_t header;
		flashRead((void *)address,(uint8_t *)&header, sizeof(header));
		if (maxBytes>=header.size)
		{
			flashRead((void *)(address+sizeof(header)),ptrData, header.size);
		} else
		{
			ERROR("The maxBytes is not enough to hold the data %" PRIu32,header.size);
			header.size=0;
		}
		return header.seconds;

	}
	return 0;

}

