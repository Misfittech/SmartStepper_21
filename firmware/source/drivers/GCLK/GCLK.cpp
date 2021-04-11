/*
 * GCLK.cpp
 *
 *  Created on: Oct 19, 2019
 *      Author: Trampas
 */

#include "GCLK.h"
#include "sam.h"
#include "drivers/delay/delay.h"
#include "libraries/syslog/syslog.h"

bool glck_sync(void)
{
	uint32_t t0 = 10000;
		while ( GCLK->SYNCBUSY.reg && t0 > 0)
		{
			/* Wait for synchronization */
			t0--;
			delay_us(1);
		}
		if (t0 == 0)
		{
			ERROR("Could not sync");
			return false;
		}
		return true;
}

bool glck_enable_clock(uint32_t clock_source, uint32_t id)
{
	GCLK->PCHCTRL[id].reg= GCLK_PCHCTRL_CHEN
			| GCLK_PCHCTRL_GEN(clock_source);
	
	return glck_sync();
}


