/*
 * @file tc.h
 * @brief
 *
 *  Created on: Dec 22, 2018
 *  @author tstern
 */


#ifndef SRC_DRIVERS_TC_TC_H_
#define SRC_DRIVERS_TC_TC_H_

#include "board.h"
#include "drivers/pin/pin.h"


typedef enum {
	TC_PRESCALER_DIV1 		= 0,
	TC_PRESCALER_DIV2 		= 1,
	TC_PRESCALER_DIV4 		= 2,
	TC_PRESCALER_DIV8 		= 3,
	TC_PRESCALER_DIV16 		= 4,
	TC_PRESCALER_DIV64 		= 5,
	TC_PRESCALER_DIV256 	= 6,
	TC_PRESCALER_DIV1024 	= 7,
}tc_prescaler_t;


class TC
{
public:
	bool setup(Tc *ptrTC, uint32_t clock_source, uint32_t clock_freq, tc_prescaler_t pre_scaler);
	bool init_periodic(voidCallback_t callback, uint32_t frequency);
	bool init_8bit_pwm();
	bool pwm(pin_t pin, uint8_t pwm); //pwm is 0-255
	void start(void);
	void stop(void);
	bool enterCritical(void);
	bool exitCritical(bool prevState);
	//bool initPWM(Tc *ptrTC, pin_t w0, pin_t w1, uint32_t clock_source, uint32_t clock_freq, uint32_t min_pwm_freq);
private:
	bool sync();
	uint32_t _clock_freq;
	bool _intEnabled;
	Tc *_ptr_tc;

	uint32_t _tc_id=0;;
};



#endif /* SRC_DRIVERS_TC_TC_H_ */
