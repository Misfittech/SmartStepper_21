/*
 * @file wdt.c
 * @brief
 *
 *  Created on: Feb 18, 2021
 *  @author tstern
 */
#include <drivers/wdt/wdt.h>

void wdtInit(wdt_cycles cycles)
{
	
	MCLK->APBAMASK.bit.WDT_=1;
	WDT->CTRLA.bit.ENABLE=0;
#if defined DEBUG
	uint32_t x;
	x=0;
	if (cycles>0)
	{
		x=cycles-1;
	}
	WDT->EWCTRL.bit.EWOFFSET=x;
	WDT->INTENSET.reg=0x01; 
	NVIC_ClearPendingIRQ(WDT_IRQn);
	NVIC_SetPriority(WDT_IRQn,(1<<__NVIC_PRIO_BITS) - 1);
	NVIC_EnableIRQ(WDT_IRQn);
#endif
	
	if (!WDT->CTRLA.bit.ALWAYSON)
	{		
		WDT->CONFIG.bit.PER=cycles;
		WDT->CTRLA.bit.ENABLE=1;
	}
	
	wdtClear();
}

void wdtClear(void) {
	if (WDT->SYNCBUSY.reg==0) {
		WDT->CLEAR.reg=WDT_CLEAR_CLEAR_KEY;
	}
	
}


#ifdef __cplusplus
extern "C" {
#endif
void WDT_Handler(void) {
	#if defined DEBUG
	  __BKPT(3);
	#endif
}

#ifdef __cplusplus
}
#endif