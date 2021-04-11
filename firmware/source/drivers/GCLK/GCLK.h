/*
 * GCLK.h
 *
 *  Created on: Oct 19, 2019
 *      Author: Trampas
 */

#ifndef SRC_DRIVERS_GCLK_GCLK_H_
#define SRC_DRIVERS_GCLK_GCLK_H_

#include <stdint.h>



bool glck_enable_clock(uint32_t clock_source, uint32_t id);


#endif /* SRC_DRIVERS_GCLK_GCLK_H_ */
