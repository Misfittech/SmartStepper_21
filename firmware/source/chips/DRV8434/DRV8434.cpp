/**********************************************************************
	Copyright (C) 2018  MisfitTech LLC,  All rights reserved.

 	MisfitTech uses a dual license model that allows the software to be used under
	a standard GPL open source license, or a commercial license.  The standard GPL
	license  requires that all software statically linked with MisfitTec Code is
	also distributed under the same GPL V2 license terms.  Details of both license
	options follow:

	- Open source licensing -
	MisfitTech is a free download and may be used, modified, evaluated and
	distributed without charge provided the user adheres to version two of the GNU
	General Public License (GPL) and does not remove the copyright notice or this
	text.  The GPL V2 text is available on the gnu.org web site

	- Commercial licensing -
	Businesses and individuals that for commercial or other reasons cannot comply
	with the terms of the GPL V2 license must obtain a low cost commercial license
	before incorporating MisfitTech code into proprietary software for distribution in
	any form.  Commercial licenses can be purchased from www.misfittech.net
	and do not require any source files to be changed.


	This code is distributed in the hope that it will be useful.  You cannot
	use MisfitTech's code unless you agree that you use the software 'as is'.
	MisfitTech's code is provided WITHOUT ANY WARRANTY; without even the implied
	warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE. MisfitTech LLC disclaims all conditions and terms, be they
	implied, expressed, or statutory.


    Written by Trampas Stern for MisfitTech.

    Misfit Tech invests time and resources providing this open source code,
    please support MisfitTech and open-source hardware by purchasing
	products from MisfitTech, www.misifittech.net!
 *********************************************************************/
#include "DRV8434.h"
#include "board.h"
#include "drivers/pin/pin.h"
#include "libraries/syslog/syslog.h"
#include "app/angle.h"
#include "libraries/sine/sine.h"

static uint8_t pinState=0;

#pragma GCC push_options
//#pragma GCC optimize ("-Ofast")




#define DAC_MAX (0x00FFL)



static inline void bridge1(int state)
{
	//PinConfig(PIN_IN3);
	//PinConfig(PIN_IN4);
	PinSetAsOutput(PIN_DVR_IN1);
	PinSetAsOutput(PIN_DVR_IN2);
	if (state==0)
	{
		PinHigh(PIN_DVR_IN1);
		PinLow(PIN_DVR_IN2);
		pinState=(pinState & 0x0C) | 0x1;
	}
	if (state==1)
	{
		PinLow(PIN_DVR_IN1);
		PinHigh(PIN_DVR_IN2);
		pinState=(pinState & 0x0C) | 0x2;
	}
	if (state==3)
	{
		PinLow(PIN_DVR_IN1);
		PinLow(PIN_DVR_IN2);
	}
}

static inline void bridge2(int state)
{
	PinSetAsOutput(PIN_DVR_IN3);
	PinSetAsOutput(PIN_DVR_IN4);
	//PinConfig(PIN_IN1);
	//PinConfig(PIN_IN2);
	if (state==0)
	{

		PinHigh(PIN_DVR_IN3);
		PinLow(PIN_DVR_IN4);
		pinState=(pinState & 0x03) | 0x4;
	}
	if (state==1)
	{
		PinLow(PIN_DVR_IN3);
		PinHigh(PIN_DVR_IN4);
		pinState=(pinState & 0x03) | 0x8;
	}
	if (state==3)
	{
		PinLow(PIN_DVR_IN3);
		PinLow(PIN_DVR_IN4);;
	}
}

void DRV8434::setDAC(uint32_t DAC1, uint32_t DAC2)
{
	DAC1=MIN(DAC1, 255);
	DAC2=MIN(DAC2, 255);
	_tc.pwm(PIN_DVR_VREFA,(uint8_t)DAC1);
	_tc.pwm(PIN_DVR_VREFB,(uint8_t)DAC2);
//	TCC1->CC[1].reg = (uint32_t)DAC1; //D9 PA07 - VREF12
//	syncTCC(TCC1);
//	TCC1->CC[0].reg = (uint32_t)DAC2; //D4 - VREF34
//	syncTCC(TCC1);
}

void DRV8434::setupDAC(void)
{
	_tc.setup((Tc *)PIN_DVR_VREFA.ptrPerherial,GENERIC_CLOCK_GENERATOR_100M,GCLK_freq[GENERIC_CLOCK_GENERATOR_100M],TC_PRESCALER_DIV4);
	_tc.init_8bit_pwm();
	PinConfig(PIN_DVR_VREFB);
	PinConfig(PIN_DVR_VREFA);
	_tc.pwm(PIN_DVR_VREFA,0);
	_tc.pwm(PIN_DVR_VREFB,0);

}


void DRV8434::begin()
{
	//setup the DRV8434 pins
	PinLow(PIN_DVR_IN1);
	PinLow(PIN_DVR_IN2);
	PinLow(PIN_DVR_IN3);
	PinLow(PIN_DVR_IN4);
	PinConfig(PIN_DVR_IN1);
	PinConfig(PIN_DVR_IN2);
	PinConfig(PIN_DVR_IN3);
	PinConfig(PIN_DVR_IN4);
	
	PinConfig(PIN_DVR_SLEEP);
	PinConfig(PIN_DVR_BDECAY);
	PinConfig(PIN_DVR_ADECAY);
	PinConfig(PIN_DVR_TOFF);
	PinConfig(PIN_DVR_FAULT);

	
	
	PinLow(PIN_DVR_VREFA);
	PinLow(PIN_DVR_VREFB);
	PinConfig(PIN_DVR_VREFA);
	PinConfig(PIN_DVR_VREFB);


	enabled=true;
	lastStepMicros=0;
	forwardRotation=true;

	setupDAC();
	//
	//	int i=0;
	//	bridge1(0);
	//	bridge2(0);
	//while (1)
	//	{
	//		int32_t x;
	//		WARNING("MA %d",i);
	//		x=(int32_t)((int64_t)i*(DAC_MAX))/3300;
	//		setDAC(x,x);
	//		delay(1000);
	//		i=i+10;
	//		if (i>1000)
	//		{
	//			i=0;
	//		}
	//
	//	}

	//
	//	WARNING("Setting DAC for 500mA output");
	//	setDAC((int32_t)((int64_t)1000*(DAC_MAX))/3300,(int32_t)((int64_t)1000*(DAC_MAX))/3300);
	//	bridge1(0);
	//	bridge2(0);
	//	while(1)
	//	{
	//
	//	}
	
	PinHigh(PIN_DVR_SLEEP);
	return;
}

void DRV8434::limitCurrent(uint8_t percent)
{
#ifdef MECHADUINO_HARDWARE
	return;
#else
	//WARNING("current limit %d",percent);
	//enableTCC0(percent);
	if (pinState & 0x01)
	{
		PinConfig(PIN_DVR_IN2);
		//pinPeripheral(PIN_A4954_IN2, PIO_TIMER_ALT); //TCC0 WO[7]
	}
	if (pinState & 0x02)
	{
		PinConfig(PIN_DVR_IN1);
		//pinPeripheral(PIN_A4954_IN1, PIO_TIMER); //TCC0 WO[1]
	}
	if (pinState & 0x04)
	{
		PinConfig(PIN_DVR_IN4);
		//pinPeripheral(PIN_A4954_IN4, PIO_TIMER_ALT);
	}
	if (pinState & 0x08)
	{
		PinConfig(PIN_DVR_IN3);
		//pinPeripheral(PIN_A4954_IN3, PIO_TIMER_ALT);
	}
#endif
}


void DRV8434::enable(bool enable)
{
	enabled=enable;
	if (enabled == false)
	{
		WARNING("A4954 disabled");
		setDAC(0,0); //turn current off
		bridge1(3); //tri state bridge outputs
		bridge2(3); //tri state bridge outputs
	}
}



//this is precise move and modulo of A4954_NUM_MICROSTEPS is a full step.
// stepAngle is in A4954_NUM_MICROSTEPS units..
// The A4954 has no idea where the motor is, so the calling function has to
// to tell the A4954 what phase to drive motor coils.
// A4954_NUM_MICROSTEPS is 256 by default so stepAngle of 1024 is 360 degrees
// Note you can only move up to +/-A4954_NUM_MICROSTEPS from where you
// currently are.
int32_t DRV8434::move(int32_t stepAngle, uint32_t mA)
{
	uint16_t angle;
	int32_t cos,sin;
	int32_t dacSin,dacCos;
	//static int i=0;
	
	mA=MIN(DRV8434_MAX_MA, mA);

	if (enabled == false)
	{
		//WARNING("A4954 disabled");
		setDAC(0,0); //turn current off
		bridge1(3); //tri state bridge outputs
		bridge2(3); //tri state bridge outputs
		return stepAngle;
	}


	//WARNING("move %d %d",stepAngle,mA);
	//handle roll overs, could do with modulo operator
	stepAngle=stepAngle%SINE_STEPS;
	if (stepAngle<0)
	{
		stepAngle += SINE_STEPS;
	}

	//figure out our sine Angle
	// note our SINE_STEPS is 4x of microsteps for a reason
	//angle=(stepAngle+(SINE_STEPS/8)) % SINE_STEPS;
	angle=(stepAngle);

	//calculate the sine and cosine of our angle
	sin=sine(angle);
	cos=cosine(angle);

	//if we are reverse swap the sign of one of the angels
	if (false == forwardRotation)
	{
		cos=-cos;
	}

	//scale sine result by current(mA)
	dacSin=((int32_t)mA*(int64_t)abs(sin))/SINE_MAX;

	//scale cosine result by current(mA)
	dacCos=((int32_t)mA*(int64_t)abs(cos))/SINE_MAX;

	//	if (i==0)
	//	{
	//		WARNING("dacs are %d %d",dacSin,dacCos);
	//	}

	//convert value into DAC scaled to 1000mA max
	dacCos=(int32_t)((int64_t)dacCos*(DAC_MAX))/DRV8434_MAX_MA;
	//convert value into DAC scaled to 1000mA max
	dacSin=(int32_t)((int64_t)dacSin*(DAC_MAX))/DRV8434_MAX_MA;

	//LOG("DACs %d %d",dacSin, dacCos);
	setDAC(dacSin,dacCos);

	if (sin != 0)
	{
		if (sin>0)
		{
			bridge1(1);
		}else
		{
			bridge1(0);
		}
	}
	if (cos != 0)
	{
		if (cos>0)
		{
			bridge2(1);
		}else
		{
			bridge2(0);
		}
	}

	//	if (i++>3000)
	//	{
	//		i=0;
	//	}
	//	YELLOW_LED(led);
	//	led=(led+1) & 0x01;
	lastStepMicros=micros();
	return stepAngle;
}
#pragma GCC pop_options

