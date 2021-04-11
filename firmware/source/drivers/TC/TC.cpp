/*
 * @file tc.cpp
 * @brief
 *
 *  Created on: Dec 22, 2018
 *  @author tstern
 */


#include <drivers/TC/TC.h>
#include "libraries/syslog/syslog.h"
#include "drivers/delay/delay.h"
#include "drivers/system/system.h"
#include "drivers/GCLK/GCLK.h"

const uint32_t divTable[8]={1,2,4,8,16,64,256,1024}; //clock division table
const Tc* _tc_inst[]=TC_INSTS;
static voidCallback_t _isr_funcs[TC_INST_NUM]={NULL};

// Wait for synchronization of registers between the clock domains
bool TC:: sync(void)
{
	uint32_t to=1000;
	while (_ptr_tc->COUNT16.SYNCBUSY.reg != 0)
	{
		if (to==0)
		{
			return false;
		}
		delay_us(1);
		to--;
	}
	return true;
}

bool TC::setup(Tc *ptrTc, uint32_t clock_source, uint32_t clock_freq, tc_prescaler_t pre_scaler)
{

	uint32_t div,i;
	ASSERT(ptrTc);
	_ptr_tc=ptrTc;

	div=pre_scaler;

	_clock_freq=clock_freq/divTable[div];

	for (i=0; i<TC_INST_NUM; i++)
	{
		if (_tc_inst[i]==_ptr_tc)
		{
			_tc_id=i;
			_isr_funcs[i]=NULL;
		}
	}

#ifdef TC0
	if (0 == _tc_id)
	{
		MCLK->APBAMASK.bit.TC0_=1;
		glck_enable_clock(clock_source, TC0_GCLK_ID);
	}
#endif
#ifdef TC1
	if (1 == _tc_id)
	{
		MCLK->APBAMASK.bit.TC1_=1;
		glck_enable_clock(clock_source, TC1_GCLK_ID);
	}
#endif
#ifdef TC2
	if (2 == _tc_id)
	{
		MCLK->APBBMASK.bit.TC2_=1;
		glck_enable_clock(clock_source, TC2_GCLK_ID);
	}
#endif
#ifdef TC3
	if (3 == _tc_id)
	{
		MCLK->APBBMASK.bit.TC3_=1;
		glck_enable_clock(clock_source, TC3_GCLK_ID);
	}
#endif
#ifdef TC4
	if (4 == _tc_id)
	{
		MCLK->APBCMASK.bit.TC4_=1;
		glck_enable_clock(clock_source, TC4_GCLK_ID);
	}
#endif
#ifdef TC5
	if (5 == _tc_id)
	{
		MCLK->APBCMASK.bit.TC5_=1;
		glck_enable_clock(clock_source, TC5_GCLK_ID);
	}
#endif
#ifdef TC6
	if (6 == _tc_id)
	{
		MCLK->APBDMASK.bit.TC6_=1;
		glck_enable_clock(clock_source, TC6_GCLK_ID);
	}
#endif
#ifdef TC7
	if (7 == _tc_id)
	{
		MCLK->APBDMASK.bit.TC7_=1;
		glck_enable_clock(clock_source, TC7_GCLK_ID);
	}
#endif

	// Disable TCx
	ptrTc->COUNT32.CTRLA.reg =0;
	ASSERT(sync());
	ptrTc->COUNT32.CTRLA.bit.PRESCALER=pre_scaler;
//	// Set Timer counter Mode to 8 bits, normal PWM, prescaler 1/256
//	ptrTc->COUNT32.CTRLA.reg |= (TC_CTRLA_MODE_COUNT32 | TC_CTRLA_PRESCALER(div));
//	ASSERT(sync());

	return true;
}

bool TC::enterCritical(void)
{
	if (NVIC_GetEnableIRQ((IRQn)(TC0_IRQn+_tc_id)))
	{
		NVIC_DisableIRQ((IRQn)(TC0_IRQn+_tc_id));
		return true;
	}
	return false;
}

bool TC::exitCritical(bool prevState)
{
	if (prevState)
	{
		NVIC_EnableIRQ((IRQn)(TC0_IRQn+_tc_id));
	}
	return true;
}
	

void TC::start(void)
{
	ASSERT(sync());
	_ptr_tc->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE; //enable counter
}

void TC::stop(void)
{
	ASSERT(sync());
	_ptr_tc->COUNT32.CTRLA.reg &= ~TC_CTRLA_ENABLE; //enable counter
}

bool TC::init_8bit_pwm(void)
{
	_ptr_tc->COUNT8.CTRLA.bit.ENABLE=0;
	_ptr_tc->COUNT8.CTRLA.bit.MODE =TC_CTRLA_MODE_COUNT8_Val;
	_ptr_tc->COUNT8.DBGCTRL.bit.DBGRUN=1; //run while debugger is halted
	ASSERT(sync());
	_ptr_tc->COUNT8.WAVE.reg=TC_WAVE_WAVEGEN_NPWM;
	_ptr_tc->COUNT8.PER.reg=255;
	_ptr_tc->COUNT8.CTRLA.bit.ENABLE=1; 
	return true;
}
bool TC::pwm(pin_t pin, uint8_t pwm)
{
	static int32_t config[2]={-1,-1};
	ASSERT(pin.id<2)
	if (config[pin.id] != (int32_t)pin.pinNum) {
		PinSetAsOutput(pin);
		PinConfig(pin);
		config[pin.id]=(int32_t)pin.pinNum;
	}
	_ptr_tc->COUNT8.CC[pin.id].reg=pwm;
	return true; 
}
	
bool TC::init_periodic(voidCallback_t callback, uint32_t frequency)
{
	uint32_t cnt;

	cnt=(_clock_freq)/frequency;
//	if (cnt>(1ul<<32))
//	{
//		ERROR("The time is greater than timer can count");
//		return false;
//	}
	
	NVIC_ClearPendingIRQ((IRQn)(TC0_IRQn+_tc_id));
	NVIC_SetPriority((IRQn)(TC0_IRQn+_tc_id),(1<<__NVIC_PRIO_BITS) - 1);
	

	_ptr_tc->COUNT32.CTRLA.reg &= ~TC_CTRLA_ENABLE; //disable counter
	_ptr_tc->COUNT32.CTRLA.bit.MODE =TC_CTRLA_MODE_COUNT32_Val;
	ASSERT(sync());

	_isr_funcs[_tc_id]=callback;

	_ptr_tc->COUNT32.INTENCLR.reg=TC_INTENCLR_MASK;
	_ptr_tc->COUNT32.INTENSET.reg=TC_INTENCLR_MC0; //trigger interrupt on CC[0] match
	_ptr_tc->COUNT32.WAVE.reg=TC_WAVE_WAVEGEN_MFRQ;

	_ptr_tc->COUNT32.COUNT.reg=0;
	ASSERT(sync());
	_ptr_tc->COUNT32.CC[0].reg=cnt;
	ASSERT(sync());
	_ptr_tc->COUNT32.CTRLBCLR.reg=0xFF;
	ASSERT(sync());
	_ptr_tc->COUNT32.CTRLA.bit.ENABLE=1; //turn timer on
	
	NVIC_EnableIRQ((IRQn)(TC0_IRQn+_tc_id));

	return true;
}


#ifdef __cplusplus
extern "C" {
#endif

#ifdef TC0
void TC0_Handler(void)
{
	uint8_t x;
	x=TC0->COUNT32.INTFLAG.reg;
	TC0->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[0])
	{
		_isr_funcs[0]();
	}
	NVIC_ClearPendingIRQ(TC0_IRQn);
}
#endif
#ifdef TC1
void TC1_Handler(void)
{
	uint8_t x;
	x=TC1->COUNT32.INTFLAG.reg;
	TC1->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[1])
	{
		_isr_funcs[1]();
	}
	NVIC_ClearPendingIRQ(TC1_IRQn);
}
#endif
#ifdef TC2
void TC2_Handler(void)
{
	uint8_t x;
	x=TC2->COUNT32.INTFLAG.reg;
	TC2->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[2])
	{
		_isr_funcs[2]();
	}
	NVIC_ClearPendingIRQ(TC2_IRQn);
}
#endif
#ifdef TC3
void TC3_Handler(void)
{
	uint8_t x;
	x=TC3->COUNT32.INTFLAG.reg;
	TC3->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[3])
	{
		_isr_funcs[3]();
	}
	NVIC_ClearPendingIRQ(TC3_IRQn);
}
#endif
#ifdef TC4
void TC4_Handler(void)
{
	uint8_t x;
	x=TC4->COUNT32.INTFLAG.reg;
	TC4->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[4])
	{
		_isr_funcs[4]();
	}
	NVIC_ClearPendingIRQ(TC4_IRQn);
}
#endif
#ifdef TC5
void TC5_Handler(void)
{
	uint8_t x;
	x=TC5->COUNT32.INTFLAG.reg;
	TC5->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[5])
	{
		_isr_funcs[5]();
	}
	NVIC_ClearPendingIRQ(TC5_IRQn);
}
#endif
#ifdef TC6
void TC6_Handler(void)
{
	uint8_t x;
	x=TC6->COUNT32.INTFLAG.reg;
	TC6->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[6])
	{
		_isr_funcs[6]();
	}
	NVIC_ClearPendingIRQ(TC6_IRQn);
}
#endif
#ifdef TC7
void TC7_Handler(void)
{
	uint8_t x;
	x=TC7->COUNT32.INTFLAG.reg;
	TC7->COUNT32.INTFLAG.reg=x; //clear flag register

	if (_isr_funcs[7])
	{
		_isr_funcs[7]();
	}
	NVIC_ClearPendingIRQ(TC7_IRQn);
}
#endif


#ifdef __cplusplus
}
#endif