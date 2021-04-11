/*
 * supc.h
 *
 *  Created on: Mar 17, 2021
 *      Author: tramp
 */

#ifndef SOURCE_DRIVERS_SUPC_SUPC_H_
#define SOURCE_DRIVERS_SUPC_SUPC_H_

#include "board.h"

#define VBOD33_LEVEL_STEP_MV (6)  //6mV Table 54-19 in datasheet

//returns true if system in brown out state
static bool isBrownOut() {return SUPC->STATUS.bit.BOD33DET;}

bool setBrownOutCallback(uint32_t milliVolts, voidCallback_t callback);



#endif /* SOURCE_DRIVERS_SUPC_SUPC_H_ */
