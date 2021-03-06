/*
 * 	Copyright (C) 2018  MisfitTech,  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Written by Trampas Stern for MisfitTech.

    Misfit Tech invests time and resources providing this open source code,
    please support MisfitTech and open-source hardware by purchasing
	products from MisfitTech, www.misifittech.net!
 */
#include "steppin.h"
#include "stepper_controller.h"
#include "board.h"
#include "drivers/GCLK/GCLK.h"

extern StepperCtrl stepperCtrl;

volatile int32_t stepsChanged=0;
volatile int64_t steps=0;


#if (PIN_STEP_INPUT != 0)
#error "this code only works with step pin being D0 (PA11, EXTINT11)"
#endif


#define WAIT_TCC2_SYNC() while(TCC2->SYNCBUSY.reg)

void checkDir(void)
{
	int dir=1;
	static int lastDir=-1;


	if (CW_ROTATION == NVM->SystemParams.dirPinRotation)
	{
		dir=0; //reverse the rotation
	}

	if (lastDir != dir)
	{
		if (dir)
		{
			EIC->CONFIG[PIN_DIR.id/8].reg &= ~(EIC_CONFIG_SENSE0_Msk<<((PIN_DIR.id%8)*4));
			EIC->CONFIG[PIN_DIR.id/8].reg |=  (EIC_CONFIG_SENSE0_HIGH_Val<<((PIN_DIR.id%8)*4));

		} else
		{
			EIC->CONFIG[PIN_DIR.id/8].reg &= ~(EIC_CONFIG_SENSE0_Msk<<((PIN_DIR.id%8)*4));
			EIC->CONFIG[PIN_DIR.id/8].reg |=  (EIC_CONFIG_SENSE0_LOW_Val<<((PIN_DIR.id%8)*4));
		}
		lastDir=dir;
	}

}

//this function can not be called in interrupt context as the overflow interrupt for tC4 needs to run.
int64_t getSteps(void)
{

//#ifndef USE_NEW_STEP
//	return 0;
//#endif
	int64_t x;
#ifdef USE_TC_STEP
	uint16_t y;
	static uint16_t lasty=0;

	TCC2->CTRLBSET.reg=TCC_CTRLBSET_CMD_READSYNC;
	WAIT_TCC2_SYNC();


	y=(uint16_t)(TCC2->COUNT.reg & 0x0FFFFul); //use only lowest 16bits
	//LOG("count is %d",y);
	steps += (int16_t)(y-lasty);
	lasty=y;

	checkDir();
	return steps;

#else
	//EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT11;
	x=stepsChanged;
	stepsChanged=0;
	//EIC->INTENSET.reg = EIC_INTENSET_EXTINT11;
	return x;
#endif
}




//this function is called on the rising edge of a step from external device
static void stepInput(void)
{
	static int dir=1;

	//read our direction pin
	dir = PinRead(PIN_DIR);

	if (CW_ROTATION == NVM->SystemParams.dirPinRotation)
	{
		dir=!dir; //reverse the rotation
	}

#ifndef USE_NEW_STEP
	stepperCtrl.requestStep(dir,1);
#else
	if (dir)
	{
		stepsChanged++;
	}else
	{
		stepsChanged--;
	}
#endif
}





#ifdef USE_TC_STEP
void setupStepEvent(void)
{
	//we will set up the EIC to generate an even on rising edge of step pin
	//make sure EIC is setup
	EICInit();


	// Assign step pin to EIC
	// Step pin is PA11, EXTINT11
	//pinPeripheral(PIN_STEP_INPUT, PIO_EXTINT);

	//set up the direction pin PA10 to trigger external interrupt
	//pinPeripheral(PIN_DIR_INPUT, PIO_EXTINT); //EXTINT10

	PinConfig(PIN_STEP);
	PinConfig(PIN_DIR);

	EIC->CTRLA.bit.ENABLE=0;
	
	while(EIC->SYNCBUSY.reg)
	{

	}
	
	//***** setup EIC ******
	EIC->EVCTRL.reg=(0x1 << PIN_STEP.id) | (0x1 << PIN_DIR.id);
	//setup up external interurpt 11 to be rising edge triggered
	//setup up external interurpt 10 to be both edge triggered
//	
	EIC->CONFIG[PIN_STEP.id/8].reg |= (EIC_CONFIG_SENSE0_RISE_Val<< ((PIN_STEP.id%8)*4));
	EIC->CONFIG[PIN_DIR.id/8].reg |= (EIC_CONFIG_SENSE0_HIGH_Val<< ((PIN_DIR.id%8)*4));
	
	EIC->CTRLA.bit.ENABLE=1;
	while(EIC->SYNCBUSY.reg)
	{

	}
	
	checkDir();

	//disable actually generating an interrupt, we only want event triggered
	EIC->INTENCLR.reg=(0x1 << PIN_STEP.id) | (0x1 << PIN_DIR.id);
//	EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT11;
//	EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT10;

	//**** setup the event system ***
	// Enable GCLK for EVSYS channel 0
	MCLK->APBBMASK.bit.EVSYS_=1;  

	glck_enable_clock(GENERIC_CLOCK_GENERATOR_MAIN, EVSYS_GCLK_ID_0);
	glck_enable_clock(GENERIC_CLOCK_GENERATOR_MAIN, EVSYS_GCLK_ID_1);
//	GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_EVSYS_0_Val));
//	while (GCLK->STATUS.bit.SYNCBUSY);
//	GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_EVSYS_1_Val));
//	while (GCLK->STATUS.bit.SYNCBUSY);

	//setup the step pin to trigger event 0 on the TCC2 (step)
	EVSYS->Channel[0].CHANNEL.reg= EVSYS_CHANNEL_EDGSEL_RISING_EDGE 
								| EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_0 + PIN_STEP.id)
								| EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

	EVSYS->USER[EVSYS_ID_USER_TCC2_EV_0].reg = 	EVSYS_USER_CHANNEL(1);

	//setup the dir pin to trigger event 2 on the TCC2 (dir change)
	EVSYS->Channel[1].CHANNEL.reg= EVSYS_CHANNEL_EDGSEL_BOTH_EDGES
								| EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_0 + PIN_DIR.id)
								| EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

	EVSYS->USER[EVSYS_ID_USER_TCC2_EV_1].reg = 	EVSYS_USER_CHANNEL(2);
								

	//**** setup the Timer counter ******
	MCLK->APBCMASK.bit.TCC2_=1; 
	//PM->APBCMASK.reg |= PM_APBCMASK_TCC2;

	// Enable GCLK for TCC2 (timer counter input clock)
//	GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_TCC2_TC3_Val));
//	while (GCLK->SYNCBUSY.reg);
	glck_enable_clock(GENERIC_CLOCK_GENERATOR_MAIN, TCC2_GCLK_ID);



	TCC2->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
	WAIT_TCC2_SYNC();

	TCC2->CTRLA.reg= TCC_CTRLA_SWRST;  //reset TCC2
	WAIT_TCC2_SYNC();
	while(TCC2->CTRLA.bit.SWRST ==1);


	//TCC2->WAVE.reg = TCC_WAVE_WAVEGEN_NFRQ;
	//WAIT_TCC2_SYNC();

	TCC2->EVCTRL.reg=TCC_EVCTRL_EVACT0_COUNTEV | TCC_EVCTRL_TCEI0
			| TCC_EVCTRL_EVACT1_DIR | TCC_EVCTRL_TCEI1;
	WAIT_TCC2_SYNC();


	TCC2->COUNT.reg=0;
	WAIT_TCC2_SYNC();

	//TCC2->CTRLBSET.bit.CMD=TCC_CTRLBSET_CMD_RETRIGGER;
	//checkDirPin();
	TCC2->CTRLBSET.bit.DIR=1;

	WAIT_TCC2_SYNC();
	TCC2->CTRLA.reg |=TCC_CTRLA_ENABLE;
	WAIT_TCC2_SYNC();


	//checkDirPin();

//
//	TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16    // Set Timer counter Mode to 16 bits
//	| TC_CTRLA_WAVEGEN_NFRQ  //normal counting mode (not using waveforms)
//	| TC_CTRLA_PRESCALER_DIV1; //count each pulse
//	WAIT_TC32_REGS_SYNC(TC4)
//
//	TC4->COUNT16.CTRLBCLR.reg=0xFF; //clear all values.
//	WAIT_TC32_REGS_SYNC(TC4)
//
//	TC4->COUNT16.EVCTRL.reg=TC_EVCTRL_TCEI | TC_EVCTRL_EVACT_COUNT; //enable event input and count
//	WAIT_TC32_REGS_SYNC(TC4)
//
//	TC4->COUNT16.COUNT.reg=0;
//	WAIT_TC32_REGS_SYNC(TC4)
//
//	TC4->COUNT16.INTENSET.bit.OVF = 1; //enable over/under flow interrupt
//	//setup the TC overflow/underflow interrupt
//	NVIC_SetPriority(TC4_IRQn, 0);
//	// Enable InterruptVector
//	NVIC_EnableIRQ(TC4_IRQn);
//
//
//	// Enable TC
//	TC4->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
//	WAIT_TC32_REGS_SYNC(TC4)
}

//static void dirChanged_ISR(void)
//{
//	int dir=0;
//	//read our direction pin
//	//dir = digitalRead(PIN_DIR_INPUT);
//	if ( (PORT->Group[g_APinDescription[PIN_DIR_INPUT].ulPort].IN.reg & (1ul << g_APinDescription[PIN_DIR_INPUT].ulPin)) != 0 )
//	{
//		dir=1;
//	}
//
//
//	if (CW_ROTATION == NVM->SystemParams.dirPinRotation)
//	{
//		dir=!dir; //reverse the rotation
//	}
//	if (dir)
//	{
//		TC4->COUNT16.CTRLBSET.bit.DIR=1;
//	} else
//	{
//		TC4->COUNT16.CTRLBCLR.bit.DIR=1;
//	}
//}
#else
void setupStepEvent(void) {}
#endif

void stepPinSetup(void)
{


#ifdef USE_TC_STEP

//	//setup the direction pin
//	dirChanged_ISR();
//
//	//attachInterrupt configures the EIC as highest priority interrupts.
//	attachInterrupt(digitalPinToInterrupt(PIN_DIR_INPUT), dirChanged_ISR, CHANGE);
	setupStepEvent();
//	NVIC_SetPriority(EIC_IRQn, 1); //set port A interrupt as highest priority


#else
	PinConfig(PIN_DIR);
	PinEnableInterrupt(PIN_STEP,RISING_EDGE,stepInput);
//	attachInterrupt(digitalPinToInterrupt(PIN_STEP_INPUT), stepInput, RISING);
//	NVIC_SetPriority(EIC_IRQn, 0); //set port A interrupt as highest priority
#endif

}
