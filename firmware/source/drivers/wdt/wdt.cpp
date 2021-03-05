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
	if (!WDT->CTRLA.bit.ALWAYSON)
	{
		WDT->CTRLA.bit.ENABLE=1;
		WDT->CONFIG.bit.PER=cycles;
	}
	wdtClear();
}

