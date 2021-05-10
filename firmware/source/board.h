/*
 * board.h
 *
 *  Created on: Jun 2, 2019
 *      Author: Trampas
 *
 *

 */

#ifndef SOURCE_BOARD_H_
#define SOURCE_BOARD_H_


#include "sam.h"
#include "drivers/system/system.h"
#include "drivers/pin/pin.h" 
#include "libraries/syslog/syslog.h"
#include "drivers/systick/systick.h"
#include "drivers/delay/delay.h"

#define FW_VERSION_STR "0.04"

#define SMARTER_STEPPER_2_8_2021


#define NZS_FAST_CAL
#define DISABLE_LCD
#define NZS_CONTROL_LOOP_HZ (6000)
#define AS5047D_SPI_CLK (1000000UL)

#define DRV8434_MAX_THERMAL_MA (1500) // maximum current of part over >3s
#define USE_TC_STEP

#define SYSLOG_RTT

#define UART_BAUD_RATE              (115200)
//I2C ADDRESSES

//this is hte lowest priority IRQ number
#define LOWEST_IRQ_PRIORITY ( (1<<__NVIC_PRIO_BITS) - 1)


//TC assignments
#define TC_PLANNER (TC3)
#define NZS_TIMER  (TC5)

//LED pins
#define GREEN_LED_PIN  (const pin_t){PIN_PB11, nullptr, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW}
#define RED_LED_PIN  (const pin_t){PIN_PA15, nullptr, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW}

#define RED_LED(x) {PinSetAsOutput(RED_LED_PIN); (x)? PinHigh(RED_LED_PIN):PinLow(RED_LED_PIN); }
#define GREEN_LED(x) {PinSetAsOutput(GREEN_LED_PIN); (x)? PinHigh(GREEN_LED_PIN):PinLow(GREEN_LED_PIN); }

//I2C pins
//#define PIN_SDA 		(const pin_t){PIN_PA12, SERCOM2, PERIPHERAL_MUX_C, 0, PERPIHERAL_GPIO_OUTPUT_HIGH}
//#define PIN_SCL 		(const pin_t){PIN_PA13, SERCOM2, PERIPHERAL_MUX_C, 1, PERPIHERAL_GPIO_OUTPUT_HIGH}

//internal pins
#define PIN_ADC_VMOT		((const pin_t){PIN_PA04, ADC0, PERPIHERAL_ADC, 4, PERPIHERAL_GPIO_INPUT})
#define PIN_USB_DETECT		((const pin_t){PIN_PA21, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT})

//user interface
#define PIN_CAL_BUTTON		((const pin_t){PIN_PB14, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT})

//interface to host
#define PIN_DIR			((const pin_t){PIN_PB12, EIC, PERPIHERAL_EIC, 12, PERPIHERAL_GPIO_INPUT})
#define PIN_STEP		((const pin_t){PIN_PB13, EIC, PERPIHERAL_EIC, 13, PERPIHERAL_GPIO_INPUT})
#define PIN_ENABLE		((const pin_t){PIN_PB08, EIC, PERPIHERAL_EIC, 8, PERPIHERAL_GPIO_INPUT})
#define PIN_ERROR_IN	((const pin_t){PIN_PA20, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT})
#define PIN_ERROR_OUT	((const pin_t){PIN_PB09, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW})

//DVR motor driver pins
#define PIN_DVR_IN1			((const pin_t){PIN_PA16, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 0 , PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_IN2			((const pin_t){PIN_PA17, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 7, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_IN3			((const pin_t){PIN_PA18, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 5, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_IN4			((const pin_t){PIN_PA19, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 6, PERPIHERAL_GPIO_INPUT_PULLED_LOW})

#define PIN_DVR_DAC_VREFA		((const pin_t){PIN_PA02, NULL, PERIPHERAL_MUX_B, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_DAC_VREFB		((const pin_t){PIN_PA05, NULL, PERIPHERAL_MUX_B, 1, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_VREFA			((const pin_t){PIN_PA07, TC1, PERIPHERAL_MUX_E, 1, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_VREFB			((const pin_t){PIN_PA06, TC1, PERIPHERAL_MUX_E, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})

#define PIN_DVR_SLEEP		((const pin_t){PIN_PA08, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_BDECAY		((const pin_t){PIN_PA09, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_ADECAY		((const pin_t){PIN_PA10, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_TOFF		((const pin_t){PIN_PA11, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_DVR_FAULT		((const pin_t){PIN_PB10, NULL, PERPIHERAL_GPIO_INPUT, 0, PERPIHERAL_GPIO_INPUT})

//encoder pins
#define PIN_AS5047D_PWR		((const pin_t){PIN_PB00, NULL, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW})
#define PIN_AS5047D_CS		((const pin_t){PIN_PB15, NULL, PERPIHERAL_GPIO_OUTPUT_HIGH, 0, PERPIHERAL_GPIO_OUTPUT_HIGH})
#define PIN_MOSI			((const pin_t){PIN_PA12, SERCOM2, PERIPHERAL_MUX_C, 0, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_SCK				((const pin_t){PIN_PA13, SERCOM2, PERIPHERAL_MUX_C, 1, PERPIHERAL_GPIO_INPUT_PULLED_LOW})
#define PIN_MISO			((const pin_t){PIN_PA14, SERCOM2, PERIPHERAL_MUX_C, 2, PERPIHERAL_GPIO_INPUT_PULLED_LOW})



/*
 *  Typedefs that are used across multiple files/modules
 */
typedef enum {
	CW_ROTATION=0,
	CCW_ROTATION=1,
} RotationDir_t;

typedef enum {
	ERROR_PIN_MODE_ENABLE=0, //error pin works like enable on step sticks
	ERROR_PIN_MODE_ACTIVE_LOW_ENABLE=1, //error pin works like enable on step sticks
	ERROR_PIN_MODE_ERROR=2,  //error pin is low when there is angle error
	ERROR_PIN_MODE_BIDIR=3,   //error pin is bidirection open collector

} ErrorPinMode_t;

typedef enum {
	CTRL_OFF =0, 	 //controller is disabled
	CTRL_OPEN=1, 	 //controller is in open loop mode
	CTRL_SIMPLE = 2, //simple error controller
	CTRL_POS_PID =3, //PID  Position controller
	CTRL_POS_VELOCITY_PID =4, //PID  Velocity controller
} feedbackCtrl_t;

/** Frequency of the board main oscillator */
#define VARIANT_MAINOSC		(32768ul)
#define VARIANT_MCK			 (120000000ul)


//this is the frequency of the GCLKs, they are set in the startup before main is called.
static const uint32_t GCLK_freq[]= {TO_MHZ(120), TO_MHZ(12), TO_MHZ(100), 32768, TO_MHZ(48), TO_MHZ(9),0,TO_MHZ(1)};

// Constants for Clock generators
#define GENERIC_CLOCK_GENERATOR_MAIN      (0u)
#define GENERIC_CLOCK_GENERATOR_12M       (1u)
#define GENERIC_CLOCK_GENERATOR_12M_SYNC   GCLK_SYNCBUSY_GENCTRL1

#define GENERIC_CLOCK_GENERATOR_OSCULP32K (2u) /* Initialized at reset for WDT */

#define GENERIC_CLOCK_GENERATOR_1M		  (8u)
#define GENERIC_CLOCK_GENERATOR_1M_SYNC	  GCLK_SYNCBUSY_GENCTRL8

#define GENERIC_CLOCK_GENERATOR_48M		  (4u)
#define GENERIC_CLOCK_GENERATOR_48M_SYNC	GCLK_SYNCBUSY_GENCTRL4

#define GENERIC_CLOCK_GENERATOR_100M	  (5u)
#define GENERIC_CLOCK_GENERATOR_100M_SYNC	GCLK_SYNCBUSY_GENCTRL5
#define GENERIC_CLOCK_GENERATOR_XOSC32K   (6u)
#define GENERIC_CLOCK_GENERATOR_XOSC32K_SYNC   GCLK_SYNCBUSY_GENCTRL6

#define GENERIC_CLOCK_GENERATOR_9M       (7u)
#define GENERIC_CLOCK_GENERATOR_9M_SYNC   GCLK_SYNCBUSY_GENCTRL7


//USE DPLL0 for 120MHZ
#define MAIN_CLOCK_SOURCE				  GCLK_GENCTRL_SRC_DPLL0




#endif /* SOURCE_BOARD_H_ */
