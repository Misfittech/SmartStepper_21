/*
 * nvsettings.h
 *
 *  Created on: Dec 6, 2017
 *      Author: tstern
 */

/**
 * This implements a nonvolatile circular storage system
 *
 */
#ifndef BREEZI_DRIVERS_NVCIRCULAR_NVCIRCULAR_H_
#define BREEZI_DRIVERS_NVCIRCULAR_NVCIRCULAR_H_

#include "board.h"


class NVCircular{
public:
	NVCircular();
	bool begin(uint32_t startAddress, uint32_t endAddress, uint32_t sectorSize);

	size_t read(uint8_t *ptrData, size_t maxBytes); //returns most recent packet
	size_t write(uint8_t *ptrData, size_t nBytes);

	//returns the time of the packet returned, zero if no packet found
	uint32_t readNextPacket(uint8_t *ptrData, size_t maxBytes, uint32_t seconds);

	//reads the last packet and returns time of packet, or zero if not found
	uint32_t readLastPacket(uint8_t *ptrData, size_t maxBytes);
	uint32_t numberPacketsNewerThan(uint32_t seconds);
	void erase(void);
private:
	typedef struct {
		uint32_t seconds;
		uint32_t address;
	}cache_t;
	bool checkData(uint32_t address);
	uint8_t getCRC(uint32_t address, size_t nBytes);
	bool getLastValidPacket(uint32_t *ptrAddress);

	uint8_t calculateCRC(uint8_t *ptrData, size_t nBytes, uint8_t crc);
	uint32_t findMostRecentSector(void);
	uint32_t findSectorJustBefore(uint32_t seconds);


	uint32_t _startAddress;
	uint32_t _endAddress;
	uint32_t _sectorSize; //is zero if the address are not vaild
	uint32_t _lastWriteAddress;
	uint32_t _lastWriteSeconds;
	uint32_t _writeIndex;

	cache_t _cache; //this is to speed up the readNext Packet


};


#endif /* BREEZI_DRIVERS_NVCIRCULAR_NVCIRCULAR_H_ */
