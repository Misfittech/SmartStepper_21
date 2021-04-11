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
#ifndef __STEPPER_CONTROLLER_H__
#define __STEPPER_CONTROLLER_H__

#include "libraries/syslog/syslog.h"
#include "board.h"
#include "chips/AS5047D/AS5047d.h"
//#include "A1333.h"
#include "calibration.h"
#include "chips/DRV8434/DRV8434.h"
//#include "A5995.h"
#include "nonvolatile.h"
#include "drivers/TC/TC.h"
//#include "fet_driver.h" //for the NEMA23 10A


#define N_DATA (1024)


typedef enum {
	STEPCTRL_NO_ERROR=0,
	STEPCTRL_NO_POWER=1, //no power to motor
	STEPCTRL_NO_CAL=2, //calibration not set
	STEPCTRL_NO_ENCODER=3, //encoder not working
} stepCtrlError_t;


typedef struct {
		int32_t Kp;
		int32_t Ki;
		int32_t Kd;
} PID_t;


 typedef __attribute__((aligned(4))) struct {
      int32_t microSecs;
      int32_t desiredLoc;
      int32_t actualLoc;
      int32_t angle;
      int32_t ma;
} Location_t;


typedef struct {
      int32_t angle;
      int32_t ma;
}Control_t;

#define MAX_NUM_LOCATIONS (64) //maximum number of locations to buffer


//this scales the PID parameters from Flash to floating point
// to fixed point int32_t values
#define CTRL_PID_SCALING (1024UL)

class StepperCtrl 
{
	private:
		volatile bool enableFeedback; //true if we are using PID control algorithm

#ifdef A1333_ENCODER
		A1333 encoder;
#else
		AS5047D encoder;
#endif

#ifdef NEMA_23_10A_HW
		FetDriver stepperDriver;
#else
#ifdef A5995_DRIVER
		A5995 stepperDriver;
#else
		DRV8434 stepperDriver;
#endif
#endif
		TC _tc; //timer for the NZS PID
		uint16_t startUpEncoder;
		volatile int32_t ticks=0;
		volatile Location_t locs[MAX_NUM_LOCATIONS];
		volatile int32_t locReadIndx=0;
		volatile int32_t locWriteIndx=0;

		volatile MotorParams_t motorParams;
		volatile SystemParams_t systemParams;
		volatile bool enabled;



		volatile int32_t loopTimeus; //time to run loop in microseconds

		volatile PID_t sPID; //simple control loop PID parameters
		volatile PID_t pPID; //positional current based PID control parameters
		volatile PID_t vPID; //velocity PID control parameters

		volatile int64_t numSteps; //this is the number of steps we have taken from our start angle

		volatile int32_t loopError;

		volatile int64_t currentLocation; //estimate of the current location from encoder feedback
		// the current location lower 16 bits is angle (0-360 degrees in 65536 steps) while upper
		// bits is the number of full rotations.

		//this is used for the velocity PID feedback
		// units are in Angles/sec where 1 Angle=360deg/65536
		volatile int64_t velocity;

		int64_t zeroAngleOffset=0;


		//volatile int16_t data[N_DATA];

		//does linear interpolation of the encoder calibration table
		int32_t getAngleCalibration(int32_t encoderAngle);

		//updates the currentMeasuredAngle with our best guess where we are
		Angle sampleAngle(void);
		Angle sampleMeanEncoder(int32_t numSamples);

		float measureStepSize(void); //steps motor and estimates step size
		uint32_t measureMaxCalibrationError(void);
		void setLocationFromEncoder(void);

		void  motorReset(void);
		void updateStep(int dir, uint16_t steps);


		bool pidFeedback(int64_t desiredLoc, int64_t currentLoc, Control_t *ptrCtrl);
		bool simpleFeedback(int64_t desiredLoc, int64_t currentLoc,Control_t *ptrCtrl);
		bool vpidFeedback(int64_t desiredLoc, int64_t currentLoc,Control_t *ptrCtrl);
		int64_t getCurrentLocation(void);

		void updateLocTable(int64_t desiredLoc, int64_t currentLoc,Control_t *ptrCtrl);

		int64_t calculatePhasePrediction(int64_t currentLoc);
		bool determineError(int64_t currentLoc, int64_t error);


	public:
		bool home_to_stall(int dir);
		bool uncalMove(int32_t steps);
		int64_t getDesiredLocation(void);
		uint16_t getStartupEncoder(void) {return startUpEncoder;}
		int32_t getLocation(Location_t *ptrLoc);

		//int32_t getSteps(void);
		Angle getEncoderAngle(void);

		void setAngle(int64_t loc);

		int64_t getZeroAngleOffset(void);
		void PrintData(void);
		void setVelocity(int64_t vel); //set velocity for vPID mode
		int64_t getVelocity(void);
		int32_t getLoopError(void) {return loopError;}; //assume atomic read

		bool calibrationValid(void) { return calTable.calValid();}  //returns true if calbiration is good

		void updateParamsFromNVM(void);  //updates the parameters from NVM
		CalibrationTable calTable;
		//void printData(void);

		bool calibrateEncoder(void); //do manual calibration of the encoder
		Angle maxCalibrationError(void); //measures the maximum calibration error as an angle

		void moveToAbsAngle(int32_t a);
		void moveToAngle(int32_t a, uint32_t ma);

		stepCtrlError_t begin(void); //returns false if we can not use motor

		bool processFeedback(void); // does the feedback loop

		feedbackCtrl_t getControlMode(void) { return systemParams.controllerMode;};

		void updateSteps(int64_t steps);
		void requestStep(int dir, uint16_t steps); //requests a step, if feedback controller is off motor does not move

		void feedback(bool enable);
		bool getFeedback(void) {return enableFeedback;}

		void encoderDiagnostics(char *ptrStr);
		int32_t measureError(void);

		//these two functions are compenstated by the zero offset
		int64_t getCurrentAngle(void);
		int64_t getDesiredAngle(void);

		void move(int dir, uint16_t steps); //forces motor to move even if feedback controller is turned off.
		void enable(bool enable);
		bool getEnable(void) {return enabled;}

		int32_t getLoopTime(void) { return loopTimeus;}

		void PID_Autotune(void);
		void setZero(void);
};

#endif //__STEPPER_CONTROLLER_H__

