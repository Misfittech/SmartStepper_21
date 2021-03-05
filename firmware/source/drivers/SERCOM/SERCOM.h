/*
 * SERCOM.h
 *
 *  Created on: May 12, 2018
 *      Author: tstern
 */

#ifndef DRIVERS_SERCOM_SERCOM_H_
#define DRIVERS_SERCOM_SERCOM_H_

#include "sam.h"

typedef bool bool_t;
typedef void (*SercomISR)(void *ptrParam);


bool disableInterrupt(Sercom *ptrSercom);
void restoreInterrupt(Sercom *ptrSercom, bool preState);
bool enableSercomNVICAndClocks(Sercom *ptrHw, SercomISR isrHandler, void *ptrCallbackParam);
uint8_t getSercomDMA_TX_ID(Sercom *ptrHw);

#endif /* DRIVERS_SERCOM_SERCOM_H_ */
