/**********************************************************************
 *      Author: tstern
 *
	Copyright (C) 2018  MisfitTech,  All rights reserved.

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
 *********************************************************************/
#include "planner.h"

#include "board.h"
//#include "wiring_private.h"
#include "libraries/syslog/syslog.h"
#include "angle.h"
#include "drivers/system/system.h"
//#include "Arduino.h"



//define the planner class as being global
Planner SmartPlanner;

void isr_planner(void)
{
	SmartPlanner.tick();
}

void Planner::begin(StepperCtrl *ptrStepper)
{

	ptrStepperCtrl=ptrStepper;
	currentMode=PLANNER_NONE;
	//setup the timer and interrupt as the last thing
	_tc.setup(TC_PLANNER,GENERIC_CLOCK_GENERATOR_48M,GCLK_freq[GENERIC_CLOCK_GENERATOR_48M],TC_PRESCALER_DIV8);
	_tc.init_periodic(isr_planner,PLANNER_UPDATE_RATE_HZ);
}

void Planner::tick(void)
{
	if (currentMode == PLANNER_NONE)
	{
		return; //do nothing
	}

	if (PLANNER_CV == currentMode)
	{
//		SerialUSB.println(currentSetAngle);
//		SerialUSB.println(endAngle);
//		SerialUSB.println(tickIncrement);
//		SerialUSB.println(fabs(currentSetAngle-endAngle));
//		SerialUSB.println(fabs(tickIncrement*2));
//		SerialUSB.println();
		int32_t x;
		if (fabs(currentSetAngle-endAngle) >= fabs(tickIncrement))
		{
			currentSetAngle+=tickIncrement;
			x=ANGLE_FROM_DEGREES(currentSetAngle);
			ptrStepperCtrl->moveToAbsAngle(x);
		}else
		{
			//we are done, make sure we end at the right point
			//SerialUSB.println("done");
			x=ANGLE_FROM_DEGREES(endAngle);
			ptrStepperCtrl->moveToAbsAngle(x);
			currentMode=PLANNER_NONE;
		}
	}


}

void Planner::stop(void)
{
	bool state;
	state = _tc.enterCritical();
	currentMode=PLANNER_NONE;
	_tc.exitCritical(state);
}

bool Planner::moveConstantVelocity(float finalAngle, float rpm)
{
	bool state;
	state = _tc.enterCritical();

	//first determine if operation is in progress
	if (PLANNER_NONE != currentMode)
	{
		//we are in operation return false
		LOG("planner operational");
		_tc.exitCritical(state);
		return false;
	}

	//get current posistion
	startAngle = ANGLE_T0_DEGREES(ptrStepperCtrl->getCurrentAngle());

	//deterime the tick increment
	tickIncrement=360.0*fabs(rpm)/60/PLANNER_UPDATE_RATE_HZ;



	//set the desired end angle
	endAngle=finalAngle;


	//set the current angle
	currentSetAngle=startAngle;

	if (startAngle>endAngle)
	{
		LOG("reverse");
		tickIncrement=-tickIncrement;
	}

//	SerialUSB.println(currentSetAngle);
//		SerialUSB.println(endAngle);
//		SerialUSB.println(tickIncrement);
//		SerialUSB.println();

	currentMode=PLANNER_CV;

	_tc.exitCritical(state);
	return true;
}
