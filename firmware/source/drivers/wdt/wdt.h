/*
 * @file wdt.h
 * @brief
 *
 *  Created on: Feb 18, 2021
 *  @author tstern
 */

#ifndef SRC_DRIVERS_WATCHDOG_WDT_H_
#define SRC_DRIVERS_WATCHDOG_WDT_H_

#include "board.h"

//watch dog timer runs off 1024hz clock
typedef enum {
	WDT_8_CYCLES,
	WDT_16_CYCLES,
	WDT_32_CYCLES,
	WDT_64_CYCLES,
	WDT_128_CYCLES,
	WDT_256_CYCLES,
	WDT_512_CYCLES,
	WDT_1024_CYCLES,
	WDT_2048_CYCLES,
	WDT_4096_CYCLES,
	WDT_8192_CYCLES,
	WDT_16384_CYCLES,	
}wdt_cycles;

void wdtInit(wdt_cycles cycles);
inline static void wdtClear(void) {WDT->CLEAR.reg=0xA5;}




#endif /* SRC_DRIVERS_WATCHDOG_WDT_H_ */
