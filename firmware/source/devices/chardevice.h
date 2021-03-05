/*
 * @file chardevice.h
 * @brief
 *
 *  Created on: Apr 13, 2019
 *  @author tstern
 */

#ifndef SRC_DEVICES_CHARDEVICE_H_
#define SRC_DEVICES_CHARDEVICE_H_



#include <inttypes.h>

class CharDevice
{
  public:
    //virtual char getChar(void)=0;
    //virtual void putChar(char c)=0;
    virtual size_t write(const uint8_t *ptrData, size_t numberBytes)=0;
    virtual size_t read(uint8_t *ptrData, size_t numberBytes)=0;
    virtual size_t available(void)=0;
    virtual void flush_tx(void)=0;
};


#endif /* SRC_DEVICES_CHARDEVICE_H_ */
