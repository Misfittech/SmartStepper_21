/*
 * TCC.h
 *
 *  Created on: Mar 16, 2021
 *      Author: tramp
 */

#ifndef SOURCE_DRIVERS_TCC_TCC_H_
#define SOURCE_DRIVERS_TCC_TCC_H_

#include "drivers/pin/pin.h"

class TCC {
public:
	bool init(uint32_t freq);
	bool pwm(pin_t pin, uint8_t percent);
private:
	uint32_t _freq=0;
	Tcc  *_ptrTcc=NULL;
	uint32_t _period; 
	bool _init(); 
};



#endif /* SOURCE_DRIVERS_TCC_TCC_H_ */
