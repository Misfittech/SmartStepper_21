/*
 * pin.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: tstern
 */

#include "pin.h"
#include "sam.h"
#include "libraries/syslog/syslog.h"
#include "drivers/system/system.h"
#include "board.h"


static std::function<void()>  InterruptCallbacks[EIC_EXTINT_NUM];
static bool_t EICInitDone=false;
#ifdef __cplusplus
extern "C" {
#endif

void EIC_0_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<0);
	if (NULL != InterruptCallbacks[0])
	{
		InterruptCallbacks[0]();
	}
}

void EIC_1_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<1);
	if (NULL != InterruptCallbacks[1])
	{
		InterruptCallbacks[1]();
	}
}

void EIC_2_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<2);
	if (NULL != InterruptCallbacks[2])
	{
		InterruptCallbacks[2]();
	}
}

void EIC_3_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<3);
	if (NULL != InterruptCallbacks[3])
	{
		InterruptCallbacks[3]();
	}
}
void EIC_4_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<4);
	if (NULL != InterruptCallbacks[4])
	{
		InterruptCallbacks[4]();
	}
}
void EIC_5_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<5);
	if (NULL != InterruptCallbacks[5])
	{
		InterruptCallbacks[5]();
	}
}
void EIC_6_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<6);
	if (NULL != InterruptCallbacks[6])
	{
		InterruptCallbacks[6]();
	}
}
void EIC_7_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<7);
	if (NULL != InterruptCallbacks[7])
	{
		InterruptCallbacks[7]();
	}
}
void EIC_8_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<8);
	if (NULL != InterruptCallbacks[8])
	{
		InterruptCallbacks[8]();
	}
}
void EIC_9_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<9);
	if (NULL != InterruptCallbacks[9])
	{
		InterruptCallbacks[9]();
	}
}
void EIC_10_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<10);
	if (NULL != InterruptCallbacks[10])
	{
		InterruptCallbacks[10]();
	}
}
void EIC_11_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<11);
	if (NULL != InterruptCallbacks[11])
	{
		InterruptCallbacks[11]();
	}
}
void EIC_12_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<12);
	if (NULL != InterruptCallbacks[12])
	{
		InterruptCallbacks[12]();
	}
}
void EIC_13_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<13);
	if (NULL != InterruptCallbacks[13])
	{
		InterruptCallbacks[13]();
	}
}
void EIC_14_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<14);
	if (NULL != InterruptCallbacks[14])
	{
		InterruptCallbacks[14]();
	}
}
void EIC_15_Handler(void)
{
	EIC->INTFLAG.reg=(1ul<<15);
	if (NULL != InterruptCallbacks[15])
	{
		InterruptCallbacks[15]();
	}
}
#ifdef __cplusplus
}
#endif

void EICInit(void)
{

	//enable clocking of the EIC using 48Mhz, max clock freq is 100Mhz for EIC
	GCLK->PCHCTRL[EIC_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_48M | (1 << GCLK_PCHCTRL_CHEN_Pos);
	MCLK->APBAMASK.bit.EIC_=1;
	ZERO_ARRAY(InterruptCallbacks);

	EIC->CTRLA.bit.SWRST=1;
	while(EIC->SYNCBUSY.reg)
	{

	}

	EIC->CTRLA.bit.ENABLE=1;
	while(EIC->SYNCBUSY.reg)
	{

	}
	EIC->INTENCLR.reg=0xFFFF;
	EICInitDone=true;
}


void PinEnableInterrupt(pin_t pin, InterruptType_t type, std::function<void()> ptrFunc)

{
	uint32_t id;
	id=pin.id;

	if (id>=EIC_EXTINT_NUM)
	{
		ERROR("Too high of EIC number");
		return;
	}
	PinConfig(pin);
	if (!EICInitDone)
	{
		EICInit();
	}

	InterruptCallbacks[id]=ptrFunc;

	EIC->CTRLA.bit.ENABLE=0;
	while(EIC->SYNCBUSY.reg)
	{

	}
	uint32_t n=id;
	if (n>8)
	{
		n = n - 8;
		n=n*4;
		EIC->CONFIG[1].reg &= ~((0x0FUL)<<n);
		EIC->CONFIG[1].reg |= ((type & 0x0F)<<n);
	} else
	{
		n=n*4;
		EIC->CONFIG[0].reg &= ~((0x0FUL)<<n);
		EIC->CONFIG[0].reg |= ((type & 0x0F)<<n);
	}

	EIC->CTRLA.bit.ENABLE=1;
	while(EIC->SYNCBUSY.reg)
	{

	}

	NVIC_DisableIRQ((IRQn_Type)(EIC_0_IRQn + id));
	NVIC_ClearPendingIRQ((IRQn_Type)(EIC_0_IRQn + id));
	NVIC_SetPriority((IRQn_Type)(EIC_0_IRQn + id), 0);
	NVIC_EnableIRQ((IRQn_Type)(EIC_0_IRQn + id));

	EIC->INTENSET.reg = 1UL<<id;

}

bool PinDisableInterrupt(pin_t pin)
{
	bool ret;
	uint32_t id;
	id=pin.id;

	if (id>=EIC_EXTINT_NUM)
	{
		ERROR("Too high of EIC number");
		return false;
	}

	ret=(EIC->INTFLAG.reg & (1 << id)) != 0;
	EIC->INTENCLR.reg = 1UL<<id;
	NVIC_DisableIRQ((IRQn_Type)(EIC_0_IRQn + id));
	NVIC_ClearPendingIRQ((IRQn_Type)(EIC_0_IRQn + id));

	InterruptCallbacks[id]=NULL;
	return ret;
}

void PinConfig(const pin_t pin)
{
	switch (pin.type)
	{

	case PERPIHERAL_GPIO_INPUT:
		PinSetAsInput(pin);
		break;
	case PERPIHERAL_GPIO_INPUT_PULLED_HIGH:
		PinSetSetInputPulledHigh(pin);
		break;
	case PERPIHERAL_GPIO_INPUT_PULLED_LOW:
		PinSetSetInputPulledLow(pin);
		break;
	case PERPIHERAL_GPIO_OUTPUT_LOW:
		PinSetAsOutput(pin);
		PinLow(pin);
		break;
	case PERPIHERAL_GPIO_OUTPUT_HIGH:
		PinSetAsOutput(pin);
		PinHigh(pin);
		break;
	case PERPIHERAL_EIC:
		PinSetAsInput(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_A);
		break;
	case PERPIHERAL_EIC_PULLED_HIGH:
		PinSetSetInputPulledHigh(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_A);
		break;
	case PERPIHERAL_EIC_PULLED_LOW:
		PinSetSetInputPulledLow(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_A);
		break;
	case PERPIHERAL_REF:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_B);
		break;
	case PERPIHERAL_ADC:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_B);
		break;
	case PERPIHERAL_AC:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_B);
		break;
	case PERPIHERAL_PTC:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_B);
		break;

	case PERPIHERAL_TC_TCC_OUTPUT:
		PinSetAsOutput(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_E);
		break;
	case PERPIHERAL_TC_TCC_INPUT:
		PinSetAsInput(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_E);
		break;
	case PERPIHERAL_TCC_OUTPUT:
		PinSetAsOutput(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_F);
		break;
	case PERPIHERAL_TCC_INPUT:
		PinSetAsInput(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_F);
		break;
	case PERPIHERAL_COM:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_G);
		break;
	case PERPIHERAL_AC_GCL:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_H);
		break;
	case PERPIHERAL_SDHC_CAN0:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_I);
		break;
	case PERPIHERAL_CORTEX:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_H);
		break;
	case PERIPHERAL_MUX_A:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_A);
		break;
	case PERIPHERAL_MUX_B:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_B);
		break;
	case PERIPHERAL_MUX_C:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_C);
		break;
	case PERIPHERAL_MUX_D:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_D);
		break;
	case PERIPHERAL_MUX_E:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_E);
		break;
	case PERIPHERAL_MUX_F:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_F);
		break;
	case PERIPHERAL_MUX_G:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_G);
		break;
	case PERIPHERAL_MUX_H:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_H);
		break;
	case PERIPHERAL_MUX_I:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_I);
		break;
	case PERIPHERAL_MUX_J:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_J);
		break;
	case PERIPHERAL_MUX_K:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_K);
		break;
	case PERIPHERAL_MUX_L:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_L);
		break;
	case PERIPHERAL_MUX_M:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_M);
		break;
	case PERIPHERAL_MUX_N:
		PinDefaults(pin);
		PinSetSetMux(pin,PIN_PERIPHERAL_MUX_N);
		break;

	}

}


