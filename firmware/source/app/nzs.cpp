/*
 * nzs.cpp
 *
 *  Created on: Dec 8, 2016
 *      Author: trampas
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

#include "nzs.h"
#include "commands.h"
#include "nonvolatile.h"
#include "angle.h"
#include "eeprom.h"
#include "steppin.h"
#include "libraries/syslog/syslog.h"
#include "libraries/libc/vsnprintf.h"
#include "drivers/wdt/wdt.h"
#include "drivers/supc/supc.h"

#pragma GCC push_options
#pragma GCC optimize ("-Ofast")

eepromData_t PowerupEEPROM={0};


volatile bool enableState=true;

int32_t dataEnabled=0;

extern StepperCtrl stepperCtrl;


int menuCalibrate(int argc, char *argv[])
{
	stepperCtrl.calibrateEncoder();
	return 0;
}

int menuTestCal(int argc, char *argv[])
{
	Angle error;
	int32_t x,y;
	char str[25];
	error=stepperCtrl.maxCalibrationError();

	x=(36000*(int32_t)error)/ANGLE_STEPS;
	LOG("Error %d %d", (int32_t)error, x);
	y=x/100;
	x=x-(y*100);
	x=abs(x);
	sprintf(str, "%d.%02d deg",y,x);
#ifndef DISABLE_LCD
	Lcd.lcdShow("Cal Error", str,"");
#endif

	LOG("Calibration error %s",str);
	return 0;
}

static  options_t stepOptions[] {
		{"200"},
		{"400"},
		{""},
};

//returns the index of the stepOptions when called
// with no arguments.
int motorSteps(int argc, char *argv[])
{
	if (argc==0)
	{
		int i;
		i=NVM->motorParams.fullStepsPerRotation;
		if (i==400)
		{
			return 1;
		}
		return 0; //default to 200
	}
	if (argc>0)
	{
		int32_t i;
		MotorParams_t params;
		memcpy((void *)&params, (void *)&NVM->motorParams, sizeof(params));
		i=atol(argv[0]);
		if (i!=params.fullStepsPerRotation)
		{
			params.fullStepsPerRotation=i;
			nvmWriteMotorParms(params);
		}
	}

	return 0;
}

static  options_t currentOptions[] {
		{"0"},
		{"100"},
		{"200"},
		{"300"},
		{"400"},
		{"500"},
		{"600"},
		{"700"},
		{"800"},
		{"900"},
		{"1000"},
		{"1100"},
		{"1200"},
		{"1300"},
		{"1400"},
		{"1500"},
		{"1600"},
		{"1700"},
		{"1800"},
		{"1900"},
		{"2000"},
		{"2100"},
		{"2200"},
		{"2300"},
		{"2400"},
		{"2500"},
		{"2600"},
		{"2700"},
		{"2800"},
		{"2900"},
		{"3000"},
		{"3100"},
		{"3200"},
		{"3300"},
		{""},
};

int motorCurrent(int argc, char *argv[])
{
	LOG("called motorCurrent %d",argc);
	if (argc==1)
	{
		int i;
		LOG("called %s",argv[0]);
		i=atol(argv[0]);
		i=i*100;
		MotorParams_t params;
		memcpy((void *)&params, (void *)&NVM->motorParams, sizeof(params));
		if (i!=params.currentMa)
		{
			params.currentMa=i;
			nvmWriteMotorParms(params);
		}
		return i/100;
	}
	int i;
	i=NVM->motorParams.currentMa/100;
	LOG(" motorCurrent return %d",i);
	return i;

}

int motorHoldCurrent(int argc, char *argv[])
{
	if (argc==1)
	{
		int i;
		i=atol(argv[0]);
		i=i*100;
		MotorParams_t params;
		memcpy((void *)&params, (void *)&NVM->motorParams, sizeof(params));
		if (i!=params.currentHoldMa)
		{
			params.currentHoldMa=i;
			nvmWriteMotorParms(params);
		}
		return i/100;
	}else
	{
		int i;
		i=NVM->motorParams.currentHoldMa/100;
		return i;
	}
}

static  options_t microstepOptions[] {
		{"1"},
		{"2"},
		{"4"},
		{"8"},
		{"16"},
		{"32"},
		{"64"},
		{"128"},
		{"256"},
		{""}
};

int microsteps(int argc, char *argv[])
{
	if (argc==1)
	{
		int i,steps;
		i=atol(argv[0]);
		SystemParams_t params;
		memcpy((void *)&params, (void *)&NVM->SystemParams, sizeof(params));
		steps=0x01<<i;
		if (steps!=params.microsteps)
		{
			params.microsteps=steps;
			nvmWriteSystemParms(params);
		}
		return i;
	}
	int i,j;
	i=NVM->SystemParams.microsteps;
	for (j=0; j<9; j++)
	{

		if ((0x01<<j) == i)
		{
			return j;
		}
	}
	return 0;
}

static  options_t controlLoopOptions[] {
		{"Off"},
		{"Open"},
		{"Simple"},
		{"Pos PID"},
		{"Vel PID"},
		{""}
};



int controlLoop(int argc, char *argv[])
{
	if (argc==1)
	{
		int i;
		i=atol(argv[0]);
		SystemParams_t params;
		memcpy((void *)&params, (void *)&NVM->SystemParams, sizeof(params));
		if (i!=params.controllerMode)
		{
			params.controllerMode=(feedbackCtrl_t)i;
			nvmWriteSystemParms(params);
		}
		return i;
	}
	return NVM->SystemParams.controllerMode;
}




#ifndef PIN_ENABLE
static  options_t errorPinOptions[] {
		{"Enable"},
		{"!Enable"}, //error pin works like enable on step sticks
		{"Error"},
		//	{"BiDir"}, //12/12/2016 not implemented yet
		{""}
};

int errorPin(int argc, char *argv[])
{
	if (argc==1)
	{
		int i;
		i=atol(argv[0]);
		SystemParams_t params;
		memcpy((void *)&params, (void *)&NVM->SystemParams, sizeof(params));
		if (i!=params.errorPinMode)
		{
			params.errorPinMode=(ErrorPinMode_t)i;
			nvmWriteSystemParms(params);
		}
		return i;
	}
	return NVM->SystemParams.errorPinMode;
}
#else

static  options_t errorPinOptions[] {
		{"Enable"},
		{"!Enable"}, //error pin works like enable on step sticks
		//      {"Error"},
		//	{"BiDir"}, //12/12/2016 not implemented yet
		{""}
};

int enablePin(int argc, char *argv[])
{
	if (argc==1)
	{
		int i;
		i=atol(argv[0]);
		SystemParams_t params;
		memcpy((void *)&params, (void *)&NVM->SystemParams, sizeof(params));
		if (i!=params.errorPinMode)
		{
			params.errorPinMode=(ErrorPinMode_t)i;
			nvmWriteSystemParms(params);
		}
		return i;
	}
	return NVM->SystemParams.errorPinMode;
}

#endif

static  options_t dirPinOptions[] {
		{"High CW"},
		{"High CCW"},
		{""}
};

int dirPin(int argc, char *argv[])
{
	if (argc==1)
	{
		int i;
		i=atol(argv[0]);
		SystemParams_t params;
		memcpy((void *)&params, (void *)&NVM->SystemParams, sizeof(params));
		if (i!=params.dirPinRotation)
		{
			params.dirPinRotation=(RotationDir_t)i;
			nvmWriteSystemParms(params);
		}
		return i;
	}
	return NVM->SystemParams.dirPinRotation;
}






//this function is called when error pin changes as enable signal
static void enableInput(void)
{
	static bool lastState=true;
#ifdef PIN_ENABLE
	if (NVM->SystemParams.errorPinMode == ERROR_PIN_MODE_ENABLE)
	{
		static int enable;
		//read our enable pin
		enable = PinRead(PIN_ENABLE);
		if (enable != enableState)
		{
			WARNING("Enable now %d",enable);
		}
		enableState=enable;
		//stepperCtrl.enable(enable);
	}
	if (NVM->SystemParams.errorPinMode == ERROR_PIN_MODE_ACTIVE_LOW_ENABLE)
	{
		static int enable;
		//read our enable pin
		enable = !PinRead(PIN_ENABLE);
		if (enable != enableState)
		{
			WARNING("Enable now %d",enable);
		}
		enableState=enable;
		//stepperCtrl.enable(enable);
	}
#else
	if (NVM->SystemParams.errorPinMode == ERROR_PIN_MODE_ENABLE)
	{
		static int enable;
		//read our enable pin
		enable = digitalRead(PIN_ERROR);
		enableState=enable;
		//stepperCtrl.enable(enable);
	}
	if (NVM->SystemParams.errorPinMode == ERROR_PIN_MODE_ACTIVE_LOW_ENABLE)
	{
		static int enable;
		//read our enable pin
		enable = !digitalRead(PIN_ERROR);
		enableState=enable;
		//stepperCtrl.enable(enable);
	}
#endif

#ifdef USE_STEP_DIR_SERIAL

	static uint8_t pinCFG[2];
	static uint8_t pinMux[2];
	if (enableState == false  && lastState==true)
	{
		// turn the step/dir to serial port

		//save pin config for restoring
		pinCFG[0]=getPinCfg(PIN_STEP_INPUT);
		pinCFG[1]=getPinCfg(PIN_DIR_INPUT);
		pinMux[0]=getPinMux(PIN_STEP_INPUT);
		pinMux[1]=getPinMux(PIN_DIR_INPUT);

		//lets see if the step pin has interrupt enabled
		if (pinMux[0] == PORT_PMUX_PMUXE_A_Val)
		{
			EExt_Interrupts in = g_APinDescription[PIN_STEP_INPUT].ulExtInt;
			EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT(1 << in); //disable the interrupt
			//we need to disable the interrupt
		}

		//now we need to set the pins to serial port peripheral (sercom0)
		setPinMux(PIN_STEP_INPUT,PORT_PMUX_PMUXE_C_Val);
		setPinMux(PIN_DIR_INPUT,PORT_PMUX_PMUXE_C_Val);

		//make sure that step pin is input with mux to peripheral
		setPinCfg(PIN_STEP_INPUT, PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN | PORT_PINCFG_PULLEN);

		//make sure that dir pin is an output with mux to peripheral
		setPinCfg(PIN_DIR_INPUT, PORT_PINCFG_PMUXEN );

		Serial1.begin(STEP_DIR_BAUD);

	}
	if (enableState == true  && lastState==false)
	{
		Serial1.end();
		setPinMux(PIN_STEP_INPUT,pinMux[0]);
		setPinMux(PIN_DIR_INPUT,pinMux[1]);
		setPinCfg(PIN_STEP_INPUT,pinCFG[0]);
		setPinCfg(PIN_DIR_INPUT,pinCFG[1]);
		//turn step/dir pins back to GPIO
		if (PORT_PMUX_PMUXE_A_Val == pinMux[0])
		{
			//if interrupt was enabled for step pin renable it.
			EExt_Interrupts in = g_APinDescription[PIN_STEP_INPUT].ulExtInt;
			EIC->INTENSET.reg = EIC_INTENCLR_EXTINT(1 << in); //enable the interrupt
		}

	}

#endif //USE_STEP_DIR_SERIAL
	lastState=enableState;
}





//void TC5_Handler()
//{
//	//	static bool led=false;
//	//	YELLOW_LED(led);
//	//	led=!led;
//	//interrupts(); //allow other interrupts
//	if (TC5->COUNT16.INTFLAG.bit.OVF == 1)
//	{
//		int error=0;
//
//
//		error=(stepperCtrl.processFeedback()); //handle the control loop
//		RED_LED(error);
//		//#ifdef PIN_ENABLE
//		//		GPIO_OUTPUT(PIN_ERROR);
//		//		bool level;
//		//		level = !NVM->SystemParams.errorLogic;
//		//		if (error)
//		//		{	//assume high is inactive and low is active on error pin
//		//			digitalWrite(PIN_ERROR,level);
//		//		}else
//		//		{
//		//			digitalWrite(PIN_ERROR,!level);
//		//		}
//		//#else
//		//
//		//		if (NVM->SystemParams.errorPinMode == ERROR_PIN_MODE_ERROR)
//		//		{
//		//			GPIO_OUTPUT(PIN_ERROR);
//		//			if (error)
//		//			{	//assume high is inactive and low is active on error pin
//		//				digitalWrite(PIN_ERROR,LOW);
//		//			}else
//		//			{
//		//				digitalWrite(PIN_ERROR,HIGH);
//		//			}
//		//		}
//		//#endif
//		TC5->COUNT16.INTFLAG.bit.OVF = 1;    // writing a one clears the flag ovf flag
//	}
//
//}

//check the NVM and set to defaults if there is any
void validateAndInitNVMParams(void)
{

	if (false == ParametersValid((void *)&NVM->sPID,sizeof(NVM->sPID)))
	{
		LOG("test");
		nvmWrite_sPID(0.2,0.002, 0.00);
	}

	if (false == ParametersValid((void *)&NVM->pPID,sizeof(NVM->pPID)))
	{
		nvmWrite_pPID(1.0, 0, 0);
	}

	if (false ==ParametersValid((void *)&NVM->vPID,sizeof(NVM->vPID)))
	{
		nvmWrite_vPID(2.0, 1.0, 1.0);
	}

	if (false == ParametersValid((void *)&NVM->SystemParams,sizeof(NVM->SystemParams)))
	{
		SystemParams_t params;
		params.microsteps=16;
		params.controllerMode=CTRL_SIMPLE;
		params.dirPinRotation=CW_ROTATION; //default to clockwise rotation when dir is high
		params.errorLimit=(int32_t)ANGLE_FROM_DEGREES(1.8);
		params.errorPinMode=ERROR_PIN_MODE_ENABLE;  //default to enable pin
		params.homePin=-1;
		params.errorLogic=false;
		params.homeAngleDelay=ANGLE_FROM_DEGREES(10);
		nvmWriteSystemParms(params);
	}

	//the motor parameters are check in the stepper_controller code
	// as that there we can auto set much of them.


}



void isr_bod33(void)
{
	eepromFlush(); //flush the eeprom
}


static void configure_bod(void)
{
	setBrownOutCallback(3000, isr_bod33);
}
void NZS::processRS485(void)
{
//	if (_rs485Coms.available()>0)
//	{
//		size_t n,x;
//		uint32_t src;
//		uint8_t data[MAX_COMM_MESSAGE_SIZE]={0};
//		n=_rs485Coms.read(&src,data,sizeof(data));
//		LOG("Read %d from %d", n,src);
//		//LOG("data: %s", data);
//		n=processCommand(src,data,n);
//	}
}
size_t NZS::sendResponse(uint32_t dst, uint8_t *ptrData, size_t len)
{
//	delay_ms(100); //wait for a bit before sending response
//	LOG("sending response of %d bytes",len);
//	return _rs485Coms.write(dst,ptrData,len);
	return 0;
}

size_t NZS::processCommand(uint32_t src, uint8_t *ptrData, size_t len)
{
	messages_t cmd;
	uint8_t resp[20];
	resp[0]=1; //send ACK
	cmd=(messages_t)ptrData[0];
	
	static uint8_t last_command; 
	static uint8_t num_last_data; 
	static uint32_t last_data[2]; 

#if 0
	switch (cmd)
	{
	case MSG_ACK_REQ: //ack request
	{
		ptrData[0]=1;  //send ACK
		return 1;
	}
	case MSG_ACK_RESPONSE: //ack
	{
		LOG("Got ACK");
		return 0;
	}
	case MSG_NACK: //nack
	{
		LOG("Got NACK");
		return 0; //what to do???
	}
	case MSG_SET_ID: //set id
	{
		uint32_t addr;
		memcpy(&addr, &ptrData[1], sizeof(uint32_t));
		LOG("Setting address %d",addr);
		//_rs485Coms.setAddress(addr);
		
		resp[1]=(uint8_t)cmd; //send ACK for set id
		memcpy(&resp[2], &addr, sizeof(uint32_t));
		sendResponse(src,resp,6);
		
		memcpy(&ptrData[2],&addr,sizeof(uint32_t));
		
		SystemParams_t params;
		memcpy((void *)&params, (void *)&NVM->SystemParams, sizeof(params));
	
		if (addr!=params.rs485_addr)
		{
			params.rs485_addr=addr;
			nvmWriteSystemParms(params);
		}
		last_command=(uint8_t)cmd;
		num_last_data=0;
		return 0; 
	}
	case MSG_GET_ID: //get id
	{
		LOG("get address %d",_rs485Coms.getAddress());
		
		resp[1]=(uint8_t)cmd; //send ACK for cmd
		resp[2]=_rs485Coms.getAddress(); //address data
		sendResponse(src,resp,3);
		last_command=(uint8_t)cmd;
		num_last_data=0;
		return 3;
	}
	case MSG_CALIBRATE: //calibrate.
	{
		
		resp[1]=(uint8_t)cmd; //send ACK for cmd
		sendResponse(src,resp,2);
		
		if(!stepperCtrl.calibrateEncoder())
		{
			//we have failed calibration
			stepperCtrl.uncalMove(250); //move backwards 250 steps
			LOG("trying calibration again");
			stepperCtrl.calibrateEncoder(); //try calibration again
		}
		LOG("Calibration DONE!");
		last_command=(uint8_t)cmd;
		num_last_data=0;
		return 2;
	}
	case MSG_TEST_CAL: //test calibration
	{
		Angle a;
		int32_t x;
		float f;

		resp[1]=(uint8_t)cmd; //send ACK for cmd
		sendResponse(src,resp,2);
		
		a=stepperCtrl.maxCalibrationError();
		x=(uint16_t)a*(int32_t)360000L/(int32_t)ANGLE_MAX;
		f=(float)x/1000.0;
		LOG("Calibration Test DONE!");
		last_command=(uint8_t)cmd;
		num_last_data=1;
		
		memcpy(&last_data[0],&f, sizeof(f));
		return 2+sizeof(float);
	}
	case MSG_SET_POS: //set position
	{
		int32_t x;
		float f;
		resp[1]=(uint8_t)cmd; //send ACK for cmd
		sendResponse(src,resp,2);
		memcpy(&f,&ptrData[1],sizeof(f));
		x=ANGLE_FROM_DEGREES(f);
		LOG("moving %d", x);
		stepperCtrl.moveToAbsAngle(x);
		LOG("Set DONE!");
		num_last_data=0;
		last_command=(uint8_t)cmd;
		return 2;
	}
	case MSG_GET_POS: //get position
	{
		int32_t x;
		float f;
		float loc;
		resp[1]=(uint8_t)cmd; //send ACK for cmd
		sendResponse(src,resp,2);
		f=ANGLE_T0_DEGREES(stepperCtrl.getLoopError());
		loc=ANGLE_T0_DEGREES(stepperCtrl.getDesiredLocation());;
		LOG("Set DONE!");
		
		last_command=(uint8_t)cmd;
		num_last_data=2;
		memcpy(&last_data[0],&f, sizeof(f));
		memcpy(&last_data[4],&loc, sizeof(loc));
		return 2+2*sizeof(float);
	}

	case MSG_HOME: //home
	{
		bool ret;
		resp[1]=(uint8_t)cmd; //send ACK for cmd
		sendResponse(src,resp,2);
		
		ret = stepperCtrl.home_to_stall(0);
		if (ret)
		{
			LOG("Home good");
			NZS_SET_STATUS(_rs485_status,NZS_STATUS_HOMED);
	
		} else
		{
			LOG("Home Bad");
			NZS_CLEAR_STATUS(_rs485_status,NZS_STATUS_HOMED);
		}
		last_command=(uint8_t)cmd;
		num_last_data=0;
		return 3;
	}
	case MSG_STATUS:
	{
		uint8_t x;
		resp[0]=1; //ack
		memcpy(&resp[1], &_rs485_status, 4);
		resp[5]=last_command;
		resp[6]=num_last_data;
		x=6;
		if (num_last_data>1)
		{
			memcpy(&resp[7], &last_data[0], 4);
			x=10;
		}
		if (num_last_data>1)
		{
			memcpy(&resp[11], &last_data[0], 4);
			x=15;
		}
		sendResponse(src,resp,x);
		return x;
	}
	
	default:
		return 0;
	}
#endif 
	return 0;
}
void NZS::begin(void)
{
	int to=20;
	stepCtrlError_t stepCtrlError;

	_rs485_status=NZS_STATUS_CLEARED;
	//set up the flash correctly on the board.
	flashInit();

	//_rs485_uart.init(RS485_BAUD_RATE, PIN_RS485_TX, PIN_RS485_RX, PIN_RS485_EN);
	//_debug_uart.init(SERIAL_BAUD, PIN_HMI_TX, PIN_HMI_RX);
	//SysLogInit(&_debug_uart, LOG_ALL);

	LOG("Power up!");

	uint32_t addr=10;
	if (ParametersValid((void *)&NVM->SystemParams,sizeof(NVM->SystemParams)))
	{
		addr=NVM->SystemParams.rs485_addr;
	}
	//_rs485Coms.init(&_rs485_uart,addr);
	
	

	//	while(1)
	//	{
	//		processRS485();
	//		delay_ms(100);
	//	}
	//
	//	while(1)
	//	{
	//		if (_rs485Coms.available()>0)
	//		{
	//			size_t n;
	//			uint32_t src;
	//			uint8_t data[MAX_COMM_MESSAGE_SIZE]={0};
	//			n=_rs485Coms.read(&src,data,sizeof(data));
	//			LOG("Read %d from %d", n,src);
	//			LOG("data: %s", data);
	//		}
	//		delay_ms(10);
	//	}
	//
	//	char str[20];
	//	uint8_t ix=0;
	//	while(1)
	//	{
	//		//_rs485_uart.write((uint8_t *)str,sizeof(str));
	//		_snprintf(str,sizeof(str),"test%d\n\r",ix);
	//		_rs485Coms.write(0,(uint8_t *)str,sizeof(str));
	//		LOG("sent packet");
	//		delay_ms(500);
	//		ix++;
	//	}


	validateAndInitNVMParams();

	LOG("EEPROM INIT");
	if (EEPROM_OK == eepromInit()) //init the EEPROM
	{
		eepromRead((uint8_t *)&PowerupEEPROM, sizeof(PowerupEEPROM));
	}
	//configure_bod(); //configure the BOD
#ifndef DISABLE_LCD
	LOG("Testing LCD");
	Lcd.begin(&stepperCtrl);

#ifdef A5995_DRIVER
	Lcd.lcdShow("MisfitTech","NEMA 23", VERSION);
#else
	Lcd.lcdShow("MisfitTech","NEMA 17", VERSION);
#endif

#endif

	LOG("command init!");
	//commandsInit(&_debug_uart,nullptr); //setup command handler system


	stepCtrlError=STEPCTRL_NO_CAL;


	while (STEPCTRL_NO_ERROR != stepCtrlError)
	{
		wdtClear();
		LOG("init the stepper controller");
		stepCtrlError=stepperCtrl.begin(); //start controller before accepting step inputs

		//todo we need to handle error on LCD and through command line
		if (STEPCTRL_NO_POWER == stepCtrlError)
		{

			LOG("Appears that there is no Motor Power");
			LOG("Connect motor power!");
#ifndef DISABLE_LCD
			Lcd.lcdShow("Waiting", "MOTOR", "POWER");
#endif
			while (STEPCTRL_NO_POWER == stepCtrlError)
			{
				processRS485(); //handle RS485 commands
				commandsProcess(); //handle commands
				stepCtrlError=stepperCtrl.begin(); //start controller before accepting step inputs
			}

		}

		if (STEPCTRL_NO_CAL == stepCtrlError)
		{
			wdtClear();
			LOG("You need to Calibrate");
			
#ifndef DISABLE_LCD
			Lcd.lcdShow("   NOT ", "Calibrated", " ");
			delay(1000);
			Lcd.setMenu(MenuCal);
			Lcd.forceMenuActive();
#endif
			//TODO add code here for LCD and command line loop
			while(false == stepperCtrl.calibrationValid())
			{
				GREEN_LED(1);
				wdtClear();
				commandsProcess(); //handle commands
				if (!PinRead(PIN_CAL_BUTTON))
				{
					stepperCtrl.calibrateEncoder();
				}
				
#ifndef DISABLE_LCD
				Lcd.process();
#endif
			}
#ifndef DISABLE_LCD
			Lcd.setMenu(NULL);
#endif
		}
		GREEN_LED(0);;

		if (STEPCTRL_NO_ENCODER == stepCtrlError)
		{
			LOG("Encoder not working");
			LOG(" try disconnecting power from board for 15+mins");
			LOG(" you might have to short out power pins to ground");
#ifndef DISABLE_LCD
			Lcd.lcdShow("Encoder", " Error!", " REBOOT");
#endif
			while(1)
			{

			}
		}

	}
#ifndef DISABLE_LCD
	Lcd.setMenu(MenuMain);
#endif

	NZS_SET_STATUS(_rs485_status,NZS_STATUS_CAL);
	
	stepPinSetup(); //setup the step pin

#ifdef PIN_ENABLE
	PinConfig(PIN_ENABLE);
	
	//because interrupt are fast and possible noise on line 
	// we do not use interrupt, we call enableInput in the loop
	// PinEnableInterrupt(PIN_ENABLE,BOTH_EDGES,enableInput);

#else
	//attachInterrupt(digitalPinToInterrupt(PIN_ERROR), enableInput, CHANGE);
#endif

	SmartPlanner.begin(&stepperCtrl);
	RED_LED(false);
	LOG("SETUP DONE!");

		
}


void printLocation(void)
{
	char buf[128]={0};
	Location_t loc;
	int32_t n, i, len;
	int32_t pktSize;

	if (dataEnabled==0)
	{
		//RED_LED(false);
		return;
	}

	//the packet length for binary print is 12bytes
	// assuming rate of 6Khz this would be 72,000 baud
	i=0;
	n=stepperCtrl.getLocation(&loc);
	if (n==-1)
	{
		//RED_LED(false);
		return;
	}

	len=0;
	pktSize=sizeof(Location_t)+1; //packet lenght is size location plus sync byte

	//     //binary write

	while(n>=0 && (len)<=(128-pktSize))
	{
		memcpy(&buf[len],&loc,sizeof(Location_t));
		len+=sizeof(Location_t);
		buf[len]=0XAA; //sync
		len++;
		buf[len]=sizeof(Location_t); //data len
		len++;

		n=stepperCtrl.getLocation(&loc);
		i++;
	}

	//SerialUSB.write(buf,len);

	//hex write
	// hex write is 29 bytes per tick, @ 6khz this 174000 baud
	//   while(n>=0 && (i*29)<(200-29))
	//   {
	//      sprintf(buf,"%s%08X\t%08X\t%08X\n\r",buf,loc.microSecs,loc.desiredLoc,loc.actualLoc);
	//      n=stepperCtrl.getLocation(&loc);
	//      i++;
	//   }
	//   SerialUSB.write(buf,strlen(buf));

	//	if (n<=0)
	//	{
	//		RED_LED(false);
	//	}else
	//	{
	//		RED_LED(true);
	//	}

	return;
}

void NZS::loop(void)
{
	eepromData_t eepromData;


	//   if (dataEnabled==0)
	//   {
	//      LOG("loop time is %dus",stepperCtrl.getLoopTime());
	//   }

	//read the enable pin and update
	// this is also done as an edge interrupt but does not always see
	// to trigger the ISR.
	enableInput();

	if (enableState != stepperCtrl.getEnable())
	{
		stepperCtrl.enable(enableState);
	}

//	//handle EEPROM
//	eepromData.angle=stepperCtrl.getCurrentAngle();
//	eepromData.encoderAngle=stepperCtrl.getEncoderAngle();
//	eepromData.valid=1;
//	eepromWriteCache((uint8_t *)&eepromData,sizeof(eepromData));

	wdtClear();
	commandsProcess(); //handle commands
	//processRS485(); //handle RS485 commands
#ifndef DISABLE_LCD
	Lcd.process();
#endif
	//stepperCtrl.PrintData(); //prints steps and angle to serial USB.


	printLocation(); //print out the current location

	return;
}

#pragma GCC pop_options
