/*
 * I2C.h
 *
 *  Created on: May 3, 2018
 *      Author: xps15
 */

#ifndef SRC_DRIVERS_I2C_I2C_MASTER_H_
#define SRC_DRIVERS_I2C_I2C_MASTER_H_

#include "sam.h"
#include "libraries/syslog/syslog.h"
#include "drivers/pin/pin.h"
#include "drivers/system/system.h"

class I2CMaster {
	public:
		bool_t init(const pin_t SDA, const pin_t SCL, uint32_t clockRate);
		size_t write(uint8_t txAddress, bool_t stopBit, uint8_t *ptrData, size_t count);
		size_t read(uint8_t txAddress, bool_t stopBit, uint8_t *ptrData, size_t count);
	private:
		Sercom *ptrSercom;
		pin_t pinSDA;
		pin_t pinSCL;
};


#endif /* SRC_DRIVERS_I2C_I2C_MASTER_H_ */
