/*
 * systick.cpp
 *
 *  Created on: Feb 4, 2018
 *      Author: xps15
 */

#include "systick.h"
#include "sam.h"
#include <memory.h>

#define SYSTICK_ISR_NUM (15)

voidCallback_t SystickCallback;

static volatile uint32_t _milliTicks=1; //start time at 1 such that time of zero is always older.
static volatile uint32_t _porSecs=1; //start time at 1 such that time of zero is always older.
static volatile uint32_t _secTicks=0;
static volatile uint32_t _factoryOffset=0;


systick_timer_t *ptrTimers=NULL;
volatile uint32_t nextTimerMillis=(uint32_t)(-1); 

static bool systickDisableIsr(void)
{
	bool ret =((SysTick->CTRL & SysTick_CTRL_TICKINT_Msk )!=0);
	SysTick->CTRL &= (~SysTick_CTRL_TICKINT_Msk);
	return ret;
}

static void systickEnableIsr(bool prevState)
{
	if (prevState)
	{
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	}
}

uint32_t getNextTimer()
{
	systick_timer_t *ptr;
	uint32_t x=(uint32_t)(-1);
	ptr=ptrTimers;
	while(ptr!=NULL)
	{
		if (ptr->_millis<x && ptr->_triggered==false)
		{
			x=ptr->_millis;
		}
		ptr=(systick_timer_t *)ptr->ptrNext;

	}
	return x;
}


bool addPeridoicTimer(systick_timer_t *ptrTimer, voidCallback_t callback, uint32_t delay_millis)
{

	bool isrState=systickDisableIsr();
	ptrTimer->_millis=millis()+delay_millis;
	ptrTimer->_callback=callback;
	ptrTimer->_oneShot=false;
	ptrTimer->_delay=delay_millis;
	
	
	ptrTimer->ptrNext=(void *)ptrTimers;
	ptrTimers=ptrTimer;
	ptrTimer->_triggered=false;
	nextTimerMillis=getNextTimer();
	systickEnableIsr(isrState);
	return true; 
}

bool addOneShotTimer(systick_timer_t *ptrTimer, voidCallback_t callback, uint32_t delay_millis)
{
	bool isrState=systickDisableIsr();
	ptrTimer->_millis=millis()+delay_millis;
	ptrTimer->_callback=callback;
	ptrTimer->_delay=delay_millis;
	ptrTimer->_oneShot=true;
	ptrTimer->ptrNext=(void *)ptrTimers;
	ptrTimers=ptrTimer;
	ptrTimer->_triggered=false;
	nextTimerMillis=getNextTimer();
	systickEnableIsr(isrState);
	return true; 
}
bool removeTimer(systick_timer_t *ptrTimer)
{
	systick_timer_t *ptr;
	bool ret=false;
	bool isrState=systickDisableIsr();
	ptr=ptrTimers;
	
	//check to see if the timer is the first timer
	while (ptr==ptrTimer && ptr!=NULL)
	{
		ptrTimers=(systick_timer_t *)ptr->ptrNext;
		ptr=ptrTimers;
		ret=true;
	}
	
	//check to see if it is next timer. 
	while(ptr!=NULL)
	{
		while(ptr->ptrNext == ptrTimer)
		{
			ptr->ptrNext=((systick_timer_t *)ptr->ptrNext)->ptrNext;
			nextTimerMillis=getNextTimer();
			ptrTimer->_triggered=true;
			ret=true;
		}
		ptr=(systick_timer_t *)ptr->ptrNext;
	}
	systickEnableIsr(isrState);
	return ret; 
}

bool rescheduleTimer(systick_timer_t *ptrTimer, uint32_t delay_millis)
{
	bool isrState=systickDisableIsr();
	ptrTimer->_millis=delay_millis+millis();
	ptrTimer->_triggered=false;	
	nextTimerMillis=getNextTimer();
	systickEnableIsr(isrState);
	return true;
}

void SysTickInit(voidCallback_t callback)
{
	SystickCallback = callback;
	// Set Systick to 1ms interval, common to all Cortex-M variants
	if (SysTick_Config(SystemCoreClock / (1000L)))
	{
		// Capture error
		while (1)
			;
	}
}



uint32_t millis(void)
{
	return _milliTicks;
}

uint32_t porSeconds(void)
{
	return _porSecs;
}

uint32_t factorySeconds(void)
{
	return _porSecs + _factoryOffset;
}

void setFactorySeconds(uint32_t sec)
{
	_factoryOffset=(sec-_porSecs);
}



uint32_t micros(void)
{
	uint32_t ticks, ticks2;
	uint32_t pend, pend2;
	uint32_t count, count2;

	ticks2  = SysTick->VAL;
	pend2   = !!(SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)  ;
	count2  = _milliTicks ;

	do
	{
		ticks=ticks2;
		pend=pend2;
		count=count2;
		ticks2  = SysTick->VAL;
		pend2   = !!(SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)  ;
		count2  = _milliTicks ;
	} while ((pend != pend2) || (count != count2) || (ticks < ticks2));

	return ((count+pend) * 1000) + (((SysTick->LOAD  - ticks)*(1048576/(F_CPU/1000000)))>>20);
}
#ifdef __cplusplus
extern "C" {
#endif
void SysTick_Handler(void)
{
	_milliTicks++;
	_secTicks++;

	if (_milliTicks>=nextTimerMillis)
	{
		systick_timer_t *ptr;
		ptr=ptrTimers;
		while(ptr !=NULL)
		{
			if (ptr->_triggered==false)
			{
				if (ptr->_millis>=_milliTicks)
				{
					ptr->_triggered=true;
					if (ptr->_callback != NULL)
					{
						(ptr->_callback)();
					}
					if (!ptr->_oneShot)
					{
						ptr->_millis=ptr->_delay+_milliTicks;
						ptr->_triggered=false;	
					}

				}
			}
			ptr=(systick_timer_t *)ptr->ptrNext;
		}
		nextTimerMillis=getNextTimer();
	}
	if (_secTicks>=1000)
	{
		_secTicks=0;
		_porSecs++;
	}
	//SystickCallback();
}
#ifdef __cplusplus
}
#endif

