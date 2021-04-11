/*
 * supc.c
 *
 *  Created on: Mar 17, 2021
 *      Author: tramp
 */


#include "supc.h"

bool _initDone=false;
voidCallback_t _callback=NULL;

static bool init(void)
{
	MCLK->APBAMASK.bit.SUPC_=1; //enable clock
	
	_initDone=true;
	return _initDone;
}

bool setBrownOutCallback(uint32_t milliVolts, voidCallback_t callback)
{
	ASSERT(callback);
	if (!_initDone) {
		init();
	}
	_callback=callback;
	
	SUPC->BOD33.reg=0;
	while(SUPC->STATUS.bit.BOD33RDY==0) {};
	SUPC->BOD33.reg=SUPC_BOD33_ACTION_INT |  //interrupt on BOD
			SUPC_BOD33_LEVEL(milliVolts/VBOD33_LEVEL_STEP_MV)|
			SUPC_BOD33_ENABLE; 
	while(SUPC->STATUS.bit.BOD33RDY==0) {};
	SUPC->INTENSET.bit.BOD33DET=1;
	
	NVIC_SetPriority(SUPC_1_IRQn, (1<<__NVIC_PRIO_BITS) - 1); 
	NVIC_EnableIRQ(SUPC_1_IRQn);
	return true;
}

#ifdef __cplusplus
extern "C" {
#endif

void SUPC_1_Handler(void)
{
	if (NULL != _callback){
		_callback();
	}
	SUPC->INTFLAG.bit.BOD33DET=1; //clear interrupt flag
}

#ifdef __cplusplus
}
#endif
