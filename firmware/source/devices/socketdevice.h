/*
 * socketdevice.h
 *
 *  Created on: Jun 3, 2020
 *      Author: Trampas
 */

#ifndef SRC_DEVICES_SOCKETDEVICE_H_
#define SRC_DEVICES_SOCKETDEVICE_H_




#include <inttypes.h>
#include <stddef.h>

class SocketDevice
{
  public:
    virtual size_t write(uint32_t id, const uint8_t *ptrData, size_t numberBytes)=0;
    virtual size_t read(uint32_t id, uint8_t *ptrData, size_t numberBytes)=0;
    virtual size_t write_size(uint32_t id)=0; //returns number of bytes we can write to socket
    virtual bool close(uint32_t id)=0;
    virtual bool active(uint32_t id)=0; //returns true if socket is active/open
};



#endif /* SRC_DEVICES_SOCKETDEVICE_H_ */
