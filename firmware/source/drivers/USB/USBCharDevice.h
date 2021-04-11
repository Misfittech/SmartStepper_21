/*
 * USBCharDevice.h
 *
 *  Created on: Mar 31, 2021
 *      Author: tramp
 */

#ifndef SOURCE_DRIVERS_USB_USBCHARDEVICE_H_
#define SOURCE_DRIVERS_USB_USBCHARDEVICE_H_

#include "devices/chardevice.h"
#include "USBAPI.h"
class USBCharDevice: public CharDevice {
	 public:
    //virtual char getChar(void)=0;
    //virtual void putChar(char c)=0;
    size_t write(const uint8_t *ptrData, size_t numberBytes) {
    	return Serial.write(ptrData,numberBytes);
    }
    size_t read(uint8_t *ptrData, size_t numberBytes){
    	return Serial.readBytes((char *)ptrData,numberBytes);
    }
    size_t available(void){
    	return Serial.available();
    }
    void flush_tx(void){    
    	return Serial.flush();
    }
};



#endif /* SOURCE_DRIVERS_USB_USBCHARDEVICE_H_ */
