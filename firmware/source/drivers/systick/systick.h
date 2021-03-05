/*
 * systick.h
 *
 *  Created on: Feb 4, 2018
 *      Author: xps15
 */

#ifndef DRIVERS_SYSTICK_SYSTICK_H_
#define DRIVERS_SYSTICK_SYSTICK_H_

#include "stdint.h"


typedef void (*voidCallback_t)(void);

//the callback will be called every 1ms based on systick timer.
void SysTickInit(voidCallback_t callback);


typedef volatile struct {
	uint32_t _millis;
	uint32_t _delay;
	voidCallback_t  _callback;
	bool _triggered;
	bool _oneShot;
	void *ptrNext;
} systick_timer_t;



 bool addPeridoicTimer(systick_timer_t *ptrTimer, voidCallback_t callback, uint32_t delay_millis);
 bool addOneShotTimer(systick_timer_t *ptrTimer, voidCallback_t callback, uint32_t delay_millis);
 bool removeTimer(systick_timer_t *ptrTimer);
 bool rescheduleTimer(systick_timer_t *ptrTimer, uint32_t delay_millis);
 
 void setFactorySeconds(uint32_t sec);
 uint32_t factorySeconds(void); //this is always increasing seconds
 uint32_t porSeconds(void);
 uint32_t micros(void);
 uint32_t millis(void);


#endif /* DRIVERS_SYSTICK_SYSTICK_H_ */
