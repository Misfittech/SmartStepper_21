/*
 * SPI.h
 *
 *  Created on: Apr 28, 2018
 *      Author: tstern
 */

#ifndef DRIVERS_SPI_SPI_H_
#define DRIVERS_SPI_SPI_H_

#include "sam.h"
#include <malloc.h> //for size_t
#include "drivers/pin/pin.h"

typedef enum {
	 SPI_MODE0=0x00,
	 SPI_MODE1=0x01,
	 SPI_MODE2=0x02,
	 SPI_MODE3=0x03,
}SPIMode_t;

typedef enum {
	MSB_FIRST=0,
	LSB_FIRST=1,
}SPIDataOrder_t;


class SPI {
public:

	bool setup(pin_t mosi, pin_t miso, pin_t sck,pin_t cs);

	bool init(uint32_t frequency,SPIMode_t mode,SPIDataOrder_t ord, uint32_t dataSize);
	size_t write(const uint8_t *ptrData, size_t bytes);
	size_t read(uint8_t *ptrData, size_t cnt);
	bool transfer(uint8_t txData, uint8_t *ptrRxData);
	size_t transfer(const uint8_t *ptrTXData, uint8_t *ptrRXData, size_t cnt);
	uint16_t transfer16(uint16_t txData);
	bool setClockRate(uint32_t frequency);
private:
	pin_t _pinMosi;
	pin_t _pinMiso;
	pin_t _pinSck;
	pin_t _pinCs;
	Sercom *_pSercom;
};




#endif /* DRIVERS_SPI_SPI_H_ */
