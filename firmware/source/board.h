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

#define FW_VERSION_STR "0.01"


//non-volatile memory periodic save rate
// assuming we have two 8k sectors, with 10k erase cycles each
// and we are saving 20 bytes each write, then
// we have around 20 years if we save flash once a minute.
// thus we have 20 years of "on time"
#define NVM_DATA_SAVE_SECONDS	(60)

//address in flash where parameters are stored
#define FLASH_PARAMETERS_ADDRESS (0xFE000)

#define FLASH_NVM_END_ADDRESS 	(0xFE000-1)
#define FLASH_NVM_START_ADDRESS (0xFA000)


#define UART_BAUD_RATE              (115200)
//I2C ADDRESSES

//for DSP library
#define FFT_SIZE (256)



//this is hte lowest priority IRQ number
#define LOWEST_IRQ_PRIORITY ( (1<<__NVIC_PRIO_BITS) - 1)

//setup debug pins, we do not use SWO but we define here anyway
//#define SWO  		(const pin_t){PIN_PB30, nullptr, PERPIHERAL_CORTEX, 0, PERPIHERAL_GPIO_OUTPUT_LOW}
#define PIN_MCU_TXD (const pin_t){PIN_PA23, SERCOM5, PERIPHERAL_MUX_D, 0, PERPIHERAL_GPIO_OUTPUT_LOW}
#define PIN_MCU_RXD (const pin_t){PIN_PA22, SERCOM5, PERIPHERAL_MUX_D, 1, PERPIHERAL_GPIO_OUTPUT_LOW}

//Power Pins
#define PWR_CNTRL  (const pin_t){PIN_PB02, nullptr, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW}

//LED pins
#define GREEN_LED_PIN  (const pin_t){PIN_PB30, nullptr, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW}
#define RED_LED_PIN  (const pin_t){PIN_PB31, nullptr, PERPIHERAL_GPIO_OUTPUT_LOW, 0, PERPIHERAL_GPIO_OUTPUT_LOW}

#define RED_LED(x) {PinSetAsOutput(RED_LED_PIN); (x)? PinHigh(RED_LED_PIN):PinLow(RED_LED_PIN); }
#define GREEN_LED(x) {PinSetAsOutput(GREEN_LED_PIN); (x)? PinHigh(GREEN_LED_PIN):PinLow(GREEN_LED_PIN); }

//I2C pins
#define PIN_SDA 		(const pin_t){PIN_PA12, SERCOM2, PERIPHERAL_MUX_C, 0, PERPIHERAL_GPIO_OUTPUT_HIGH}
#define PIN_SCL 		(const pin_t){PIN_PA13, SERCOM2, PERIPHERAL_MUX_C, 1, PERPIHERAL_GPIO_OUTPUT_HIGH}



#define MCLK0_GCLK	(const pin_t){PIN_PA15, GCLK, PERIPHERAL_MUX_M, 0, PERPIHERAL_GPIO_OUTPUT_HIGH}



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
