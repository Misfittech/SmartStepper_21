/*
 * SERCOM.cpp
 *
 *  Created on: May 12, 2018
 *      Author: tstern
 */

#include "SERCOM.h"
#include "board.h"
#include "drivers/delay/delay.h"
#include "libraries/syslog/syslog.h"

void *CallbackParameters[SERCOM_INST_NUM]={nullptr};
SercomISR Callbacks[SERCOM_INST_NUM]={nullptr};

__STATIC_INLINE bool_t NVIC_IsEnabledIRQ(IRQn_Type IRQn)
{
	return ((NVIC->ISER[0] & (1 << ((uint32_t)(IRQn) & 0x1F)))!=0);
}


static IRQn_Type getIRQNumber(Sercom *ptrSercom, uint32_t bit)
{

#if defined(__SAMD51__)
	if( ptrSercom == SERCOM0)
	{
		if (bit==0) return SERCOM0_0_IRQn;
		if (bit==1) return SERCOM0_1_IRQn;
		if (bit==2) return SERCOM0_2_IRQn;
		if (bit>=3) return SERCOM0_3_IRQn;
	}
	else if(ptrSercom== SERCOM1)
	{
		if (bit==0) return SERCOM1_0_IRQn;
		if (bit==1) return SERCOM1_1_IRQn;
		if (bit==2) return SERCOM1_2_IRQn;
		if (bit>=3) return SERCOM1_3_IRQn;
	}
	else if(ptrSercom == SERCOM2)
	{
		if (bit==0) return SERCOM2_0_IRQn;
		if (bit==1) return SERCOM2_1_IRQn;
		if (bit==2) return SERCOM2_2_IRQn;
		if (bit>=3) return SERCOM2_3_IRQn;
	}
	else if(ptrSercom== SERCOM3)
	{
		if (bit==0) return SERCOM3_0_IRQn;
		if (bit==1) return SERCOM3_1_IRQn;
		if (bit==2) return SERCOM3_2_IRQn;
		if (bit>=3) return SERCOM3_3_IRQn;;
	}
	else if(ptrSercom == SERCOM4)
	{
		if (bit==0) return SERCOM4_0_IRQn;
		if (bit==1) return SERCOM4_1_IRQn;
		if (bit==2) return SERCOM4_2_IRQn;
		if (bit>=3) return SERCOM4_3_IRQn;
	}
	else if(ptrSercom == SERCOM5)
	{
		if (bit==0) return SERCOM5_0_IRQn;
		if (bit==1) return SERCOM5_1_IRQn;
		if (bit==2) return SERCOM5_2_IRQn;
		if (bit>=3) return SERCOM5_3_IRQn;
	}
#ifdef SERCOM6
	else if(ptrSercom == SERCOM6)
	{
		if (bit==0) return SERCOM5_0_IRQn;
		if (bit==1) return SERCOM5_1_IRQn;
		if (bit==2) return SERCOM5_2_IRQn;
		if (bit>=3) return SERCOM5_3_IRQn;
	}
#endif
#ifdef SERCOM7
	else if(ptrSercom == SERCOM7)
	{
		if (bit==0) return SERCOM5_0_IRQn;
		if (bit==1) return SERCOM5_1_IRQn;
		if (bit==2) return SERCOM5_2_IRQn;
		if (bit>=3) return SERCOM5_3_IRQn;
	}
#endif
#else
	if( ptrSercom == SERCOM0)
	{
		return SERCOM0_0_IRQn;
	}
	else if(ptrSercom== SERCOM1)
	{
		return  SERCOM1_0_IRQn;
	}
	else if(ptrSercom == SERCOM2)
	{
		return  SERCOM2_0_IRQn;
	}
	else if(ptrSercom== SERCOM3)
	{
		return  SERCOM3_0_IRQn;
	}
	else if(ptrSercom == SERCOM4)
	{
		return  SERCOM4_0_IRQn;
	}
	else if(ptrSercom == SERCOM5)
	{
		return  SERCOM5_0_IRQn;
	}
#endif
	ERROR("Invalid serial com");
	return PendSV_IRQn; // Dummy init to intercept potential error later
}

bool disableInterrupt(Sercom *ptrSercom)
{
	IRQn_Type num;
	bool ret;
	num=getIRQNumber(ptrSercom,0);
	ret=true;
	//ret=NVIC_IsEnabledIRQ(num);
	if (ret)
	{
#if defined(__SAMD51__)
		uint32_t i;
		for(i=0; i<4; i++)
		{
			NVIC_DisableIRQ((IRQn_Type)((uint32_t)num+i));
		}
#else
		NVIC_DisableIRQ(num);
#endif
	}
	return ret;
}

void restoreInterrupt(Sercom *ptrSercom, bool_t preState)
{
	IRQn_Type num;
	if (preState)
	{
		num=getIRQNumber(ptrSercom,0);
#if defined(__SAMD51__)
		uint32_t i;
		for(i=0; i<4; i++)
		{
			NVIC_EnableIRQ((IRQn_Type)((uint32_t)num+i));
		}
#else
		NVIC_EnableIRQ(num);
#endif
	}
}

uint8_t getSercomDMA_TX_ID(Sercom *ptrHw)
{
    if (ptrHw == SERCOM0)
    {
	return SERCOM0_DMAC_ID_TX;
    }
    if (ptrHw == SERCOM1)
    {
	return SERCOM1_DMAC_ID_TX;
    }
    if (ptrHw == SERCOM2)
    {
	return SERCOM2_DMAC_ID_TX;
    }
    if (ptrHw == SERCOM3)
    {
	return SERCOM3_DMAC_ID_TX;
    }
    if (ptrHw == SERCOM4)
    {
	return SERCOM4_DMAC_ID_TX;
    }
    if (ptrHw == SERCOM5)
    {
	return SERCOM5_DMAC_ID_TX;
    }
    ERROR("Unknown sercom");
    return 0;
}


bool enableSercomNVICAndClocks(Sercom *ptrHw, SercomISR isrHandler, void *ptrCallbackParam)
{
	uint32_t gClkId;
	uint32_t num;
	ASSERT(ptrHw);

#if defined(__SAMD51__)
	uint32_t slowClock;
	uint32_t isr_priority=0;//(1<<__NVIC_PRIO_BITS) - 1;

	if (ptrHw == SERCOM0)
	{
		gClkId =SERCOM0_GCLK_ID_CORE;
		slowClock=SERCOM0_GCLK_ID_SLOW;

		NVIC_ClearPendingIRQ(SERCOM0_0_IRQn);
		NVIC_ClearPendingIRQ(SERCOM0_1_IRQn);
		NVIC_ClearPendingIRQ(SERCOM0_2_IRQn);
		NVIC_ClearPendingIRQ(SERCOM0_3_IRQn);

		NVIC_SetPriority (SERCOM0_0_IRQn, isr_priority);  /* set Priority */
		NVIC_SetPriority (SERCOM0_1_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM0_2_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM0_3_IRQn, isr_priority);


		MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM0;
		num=0;
	}
	if (ptrHw == SERCOM1)
	{
		gClkId =SERCOM1_GCLK_ID_CORE;
		slowClock=SERCOM1_GCLK_ID_SLOW;

		NVIC_ClearPendingIRQ(SERCOM1_0_IRQn);
		NVIC_ClearPendingIRQ(SERCOM1_1_IRQn);
		NVIC_ClearPendingIRQ(SERCOM1_2_IRQn);
		NVIC_ClearPendingIRQ(SERCOM1_3_IRQn);

		NVIC_SetPriority (SERCOM1_0_IRQn, isr_priority);  /* set Priority */
		NVIC_SetPriority (SERCOM1_1_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM1_2_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM1_3_IRQn, isr_priority);


		MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM1;
		num=1;
	}
	if (ptrHw == SERCOM2)
	{
		gClkId = SERCOM2_GCLK_ID_CORE;
		slowClock = SERCOM2_GCLK_ID_SLOW;

		NVIC_ClearPendingIRQ(SERCOM2_0_IRQn);
		NVIC_ClearPendingIRQ(SERCOM2_1_IRQn);
		NVIC_ClearPendingIRQ(SERCOM2_2_IRQn);
		NVIC_ClearPendingIRQ(SERCOM2_3_IRQn);

		NVIC_SetPriority (SERCOM2_0_IRQn, isr_priority);  /* set Priority */
		NVIC_SetPriority (SERCOM2_1_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM2_2_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM2_3_IRQn, isr_priority);


		MCLK->APBBMASK.reg |= MCLK_APBBMASK_SERCOM2;
		num=2;
	}
	if (ptrHw == SERCOM3)
	{
		gClkId = SERCOM3_GCLK_ID_CORE;
		slowClock = SERCOM3_GCLK_ID_SLOW;

		NVIC_ClearPendingIRQ(SERCOM3_0_IRQn);
		NVIC_ClearPendingIRQ(SERCOM3_1_IRQn);
		NVIC_ClearPendingIRQ(SERCOM3_2_IRQn);
		NVIC_ClearPendingIRQ(SERCOM3_3_IRQn);

		NVIC_SetPriority (SERCOM3_0_IRQn, isr_priority);  /* set Priority */
		NVIC_SetPriority (SERCOM3_1_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM3_2_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM3_3_IRQn, isr_priority);


		MCLK->APBBMASK.reg |= MCLK_APBBMASK_SERCOM3;
		num=3;
	}
	if (ptrHw == SERCOM4)
	{
		gClkId = SERCOM4_GCLK_ID_CORE;
		slowClock = SERCOM4_GCLK_ID_SLOW;

		NVIC_ClearPendingIRQ(SERCOM4_0_IRQn);
		NVIC_ClearPendingIRQ(SERCOM4_1_IRQn);
		NVIC_ClearPendingIRQ(SERCOM4_2_IRQn);
		NVIC_ClearPendingIRQ(SERCOM4_3_IRQn);

		NVIC_SetPriority (SERCOM4_0_IRQn, isr_priority);  /* set Priority */
		NVIC_SetPriority (SERCOM4_1_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM4_2_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM4_3_IRQn, isr_priority);


		MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM4;
		num=4;
	}
	if (ptrHw == SERCOM5)
	{
		gClkId = SERCOM5_GCLK_ID_CORE;
		slowClock = SERCOM5_GCLK_ID_SLOW;

		NVIC_ClearPendingIRQ(SERCOM5_0_IRQn);
		NVIC_ClearPendingIRQ(SERCOM5_1_IRQn);
		NVIC_ClearPendingIRQ(SERCOM5_2_IRQn);
		NVIC_ClearPendingIRQ(SERCOM5_3_IRQn);

		NVIC_SetPriority (SERCOM5_0_IRQn, isr_priority);  /* set Priority */
		NVIC_SetPriority (SERCOM5_1_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM5_2_IRQn, isr_priority);
		NVIC_SetPriority (SERCOM5_3_IRQn, isr_priority);


		MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM5;
		num=5;
	}
#else
	{
		gClkId=GCLK_CLKCTRL_ID_SERCOM0_CORE_Val;
		nvicId=SERCOM0_0_IRQn;
		pmValue=PM_APBCMASK_SERCOM0;
		num=0;
	}
	if (ptrHw == SERCOM1)
	{
		gClkId=GCLK_CLKCTRL_ID_SERCOM1_CORE_Val;
		nvicId=SERCOM1_IRQn;
		pmValue=PM_APBCMASK_SERCOM1;
		num=1;
	}
	if (ptrHw == SERCOM2)
	{
		gClkId=GCLK_CLKCTRL_ID_SERCOM2_CORE_Val;
		nvicId=SERCOM2_IRQn;
		pmValue=PM_APBCMASK_SERCOM2;
		num=2;
	}
	if (ptrHw == SERCOM3)
	{
		gClkId=GCLK_CLKCTRL_ID_SERCOM3_CORE_Val;
		nvicId=SERCOM3_IRQn;
		pmValue=PM_APBCMASK_SERCOM3;
		num=3;
	}
	if (ptrHw == SERCOM4)
	{
		gClkId=GCLK_CLKCTRL_ID_SERCOM4_CORE_Val;
		nvicId=SERCOM4_IRQn;
		pmValue=PM_APBCMASK_SERCOM4;
		num=4;
	}
	if (ptrHw == SERCOM5)
	{
		gClkId=GCLK_CLKCTRL_ID_SERCOM5_CORE_Val;
		nvicId=SERCOM5_IRQn;
		pmValue=PM_APBCMASK_SERCOM5;
		num=5;
	}
#endif

	Callbacks[num]=isrHandler;
	CallbackParameters[num]=ptrCallbackParam;

#if defined(__SAMD51__)
	  GCLK->PCHCTRL[gClkId].reg = GENERIC_CLOCK_GENERATOR_48M | (1 << GCLK_PCHCTRL_CHEN_Pos);
	  GCLK->PCHCTRL[slowClock].reg = GENERIC_CLOCK_GENERATOR_1M | (1 << GCLK_PCHCTRL_CHEN_Pos);
	  restoreInterrupt(ptrHw,true);
	  return true;

#else
	PM->APBCMASK.reg |= pmValue; //no sync

	// Setting NVIC
	NVIC_EnableIRQ(nvicId);
	NVIC_SetPriority (nvicId, isr_priority);  /* set Priority */

	//Setting clock
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( gClkId ) | // Generic Clock 0 (SERCOMx)
			GCLK_CLKCTRL_GEN(0) | // Generic Clock Generator 0 is source
			GCLK_CLKCTRL_CLKEN ;

	uint32_t t0=1000;
	while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY && t0>0)
	{
		/* Wait for synchronization */
		t0--;
		_delay_us(10);
	}
	if (t0==0)
	{
		ERROR("Could not sync");
	}
	return (t0!=0);
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

void SERCOM0_0_Handler(void)
{
	if (Callbacks[0] != nullptr)
	{
		Callbacks[0](CallbackParameters[0]);
	}
}
void SERCOM0_1_Handler(void)
{
	if (Callbacks[0] != nullptr)
	{
		Callbacks[0](CallbackParameters[0]);
	}
}
void SERCOM0_2_Handler(void)
{
	if (Callbacks[0] != nullptr)
	{
		Callbacks[0](CallbackParameters[0]);
	}
}
void SERCOM0_3_Handler(void)
{
	if (Callbacks[0] != nullptr)
	{
		Callbacks[0](CallbackParameters[0]);
	}
}

void SERCOM1_Handler(void)
{
	if (Callbacks[1] != nullptr)
	{
		Callbacks[1](CallbackParameters[1]);
	}
}

void SERCOM2_Handler(void)
{
	if (Callbacks[2] != nullptr)
	{
		Callbacks[2](CallbackParameters[2]);
	}
}
void SERCOM3_Handler(void)
{
	if (Callbacks[3] != nullptr)
	{
		Callbacks[3](CallbackParameters[3]);
	}
}
void SERCOM4_Handler(void)
{
	if (Callbacks[4] != nullptr)
	{
		Callbacks[4](CallbackParameters[4]);
	}
}
void SERCOM5_0_Handler(void)
{
	if (Callbacks[5] != nullptr)
	{
		Callbacks[5](CallbackParameters[5]);
	}
}

void SERCOM5_1_Handler(void)
{
	if (Callbacks[5] != nullptr)
	{
		Callbacks[5](CallbackParameters[5]);
	}
}
void SERCOM5_2_Handler(void)
{
	if (Callbacks[5] != nullptr)
	{
		Callbacks[5](CallbackParameters[5]);
	}
}
void SERCOM5_3_Handler(void)
{
	if (Callbacks[5] != nullptr)
	{
		Callbacks[5](CallbackParameters[5]);
	}
}


#ifdef __cplusplus
}
#endif
