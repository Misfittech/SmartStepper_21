/*
 * TCC.cpp
 *
 *  Created on: Mar 16, 2021
 *      Author: tramp
 */

#include "TCC.h"
#include "board.h"

bool TCC::init(uint32_t freq)
{
	_freq=freq;
	_ptrTcc=NULL;
	return true;
}

static void syncTCC(Tcc *ptrPer)
{
	while(ptrPer->SYNCBUSY.reg)
	{
		
	}
}
bool TCC::_init()
{
	Tcc * tccs[]=TCC_INSTS;
	int32_t tcc_num=-1; 
	uint32_t i;
	
	if (NULL == _ptrTcc) {
		ERROR("no TCC selected");
		return false; 
	}
	
	for (i=0; i<TCC_INST_NUM; i++)
	{
		if (tccs[i]==_ptrTcc){
			tcc_num=i;
			break;
		}
	}
	ASSERT(tcc_num>=0);
	_ptrTcc=tccs[tcc_num];
#ifdef TCC0
	if (0==tcc_num)
	{
		MCLK->APBBMASK.bit.TCC0_=1;
		GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_MAIN | (1 << GCLK_PCHCTRL_CHEN_Pos);
	}
#endif 
	
#ifdef TCC1
	if (1==tcc_num)
	{
		MCLK->APBBMASK.bit.TCC1_=1;
		GCLK->PCHCTRL[TCC1_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_MAIN | (1 << GCLK_PCHCTRL_CHEN_Pos);
	}
#endif 
	
#ifdef TCC2
	if (2==tcc_num)
	{
		MCLK->APBCMASK.bit.TCC2_=1;
		GCLK->PCHCTRL[TCC2_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_MAIN | (1 << GCLK_PCHCTRL_CHEN_Pos);
	}
#endif 
	
#ifdef TCC3
	if (3==tcc_num)
	{
		MCLK->APBCMASK.bit.TCC3_=1;
		GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_MAIN | (1 << GCLK_PCHCTRL_CHEN_Pos);
	}
#endif 
#ifdef TCC4
	if (4==tcc_num)
	{
		MCLK->APBDMASK.bit.TCC4_=1;
		GCLK->PCHCTRL[TCC4_GCLK_ID].reg = GENERIC_CLOCK_GENERATOR_MAIN | (1 << GCLK_PCHCTRL_CHEN_Pos);
	}
#endif 
	while ( GCLK->SYNCBUSY.reg != 0 ) ;
	
	_ptrTcc->CTRLA.reg=0;
	_ptrTcc->CTRLA.reg=TCC_CTRLA_SWRST;
	syncTCC(_ptrTcc);
	
	//setup clock rate for counter
	if (0==_freq) {
		//run as fast as possible 
		_ptrTcc->PER.reg=100; //allow 0-100% steps in PWM 
		_period=100;
	}else
	{
		_period=VARIANT_MCK/_freq;
		_ptrTcc->PER.reg=_period; 
	}
	return true;
}

bool TCC::pwm(pin_t pin, uint8_t percent)
{
	if (NULL==_ptrTcc){
		_ptrTcc=(Tcc *)pin.ptrPerherial;
		_init();
	}
	if (_ptrTcc != (Tcc *)pin.ptrPerherial)
	{
		ERROR("Wrong TCC timer");
		return false;
	}
	
	if (percent>100) {
		percent =100; 
	}
	_ptrTcc->CC[pin.id].reg=(_period*100)/percent; 
	return true;
}