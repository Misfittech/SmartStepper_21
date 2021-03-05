/*
 * blockdevice.h
 *
 *  Created on: Jan 3, 2020
 *      Author: trampas
 */

#ifndef SRC_DEVICES_BLOCKDEVICE_H_
#define SRC_DEVICES_BLOCKDEVICE_H_




#include <inttypes.h>

class BlockDevice
{
  public:
    virtual size_t write(size_t address, const uint8_t *ptrData, size_t numberBytes)=0;
    virtual size_t read(size_t address, uint8_t *ptrData, size_t numberBytes)=0;
};



#endif /* SRC_DEVICES_BLOCKDEVICE_H_ */
