/*
 * pin.h
 *
 *  Created on: Apr 28, 2018
 *      Author: tstern
 */

#ifndef DRIVERS_PIN_PIN_H_
#define DRIVERS_PIN_PIN_H_

#include "sam.h"
#include <stdbool.h>
#include "drivers/system/system.h"


typedef enum {
	PIN_PERIPHERAL_MUX_A=0,
	PIN_PERIPHERAL_MUX_B=1,
	PIN_PERIPHERAL_MUX_C=2,
	PIN_PERIPHERAL_MUX_D=3,
	PIN_PERIPHERAL_MUX_E=4,
	PIN_PERIPHERAL_MUX_F=5,
	PIN_PERIPHERAL_MUX_G=6,
	PIN_PERIPHERAL_MUX_H=7,
	PIN_PERIPHERAL_MUX_I=8,
	PIN_PERIPHERAL_MUX_J=9,
	PIN_PERIPHERAL_MUX_K=0x0A,
	PIN_PERIPHERAL_MUX_L=0x0B,
	PIN_PERIPHERAL_MUX_M=0x0C,
	PIN_PERIPHERAL_MUX_N=0x0D,
} pinPeripheralMux_t;



typedef enum {
	PERPIHERAL_GPIO_INPUT,
	PERPIHERAL_GPIO_INPUT_PULLED_HIGH,
	PERPIHERAL_GPIO_INPUT_PULLED_LOW,
	PERPIHERAL_GPIO_OUTPUT_LOW,
	PERPIHERAL_GPIO_OUTPUT_HIGH,
	PERPIHERAL_EIC,
	PERPIHERAL_EIC_PULLED_HIGH,
	PERPIHERAL_EIC_PULLED_LOW,
	PERPIHERAL_REF,
	PERPIHERAL_ADC,
	PERPIHERAL_AC,
	PERPIHERAL_PTC,
	PERPIHERAL_TC_TCC_OUTPUT,
	PERPIHERAL_TC_TCC_INPUT,
	PERPIHERAL_TCC_OUTPUT,
	PERPIHERAL_TCC_INPUT,
	PERPIHERAL_COM,
	PERPIHERAL_AC_GCL,
	PERPIHERAL_SDHC_CAN0,
	PERPIHERAL_CORTEX,
	PERIPHERAL_MUX_A,
	PERIPHERAL_MUX_B,
	PERIPHERAL_MUX_C,
	PERIPHERAL_MUX_D,
	PERIPHERAL_MUX_E,
	PERIPHERAL_MUX_F,
	PERIPHERAL_MUX_G,
	PERIPHERAL_MUX_H,
	PERIPHERAL_MUX_I,
	PERIPHERAL_MUX_J,
	PERIPHERAL_MUX_K,
	PERIPHERAL_MUX_L,
	PERIPHERAL_MUX_M,
	PERIPHERAL_MUX_N,
}pinMuxType_t;

typedef struct {
	uint32_t pinNum;
	void *ptrPerherial;
	pinMuxType_t type;
	uint32_t id; //pad pin waveform
	pinMuxType_t sleepType;
}pin_t;

typedef enum {
	NONE=0,
	RISING_EDGE,
	FALLING_EDGE,
	BOTH_EDGES,
	HIGH_LEVEL,
	LOW_LEVEL,
}InterruptType_t;


void PinEnableInterrupt(pin_t pin, InterruptType_t type, voidCallback_t callback);
bool PinDisableInterrupt(pin_t pin); //returns true if interrupt was enabled
void PinConfig(const pin_t pin);

void EICInit(void);

#define PIN_GET_PORT(x) ((x)>>5)
#define PIN_GET_PIN(x) ((x)&0x01F)


static inline bool PinRead(const pin_t pin) __attribute__((always_inline));
static inline bool PinRead (const pin_t pin)
{
	return (PORT->Group[PIN_GET_PORT(pin.pinNum)].IN.reg & (1<<PIN_GET_PIN(pin.pinNum)))!=0;

}


static inline void PinDefaults (const pin_t pin) __attribute__((always_inline));
static inline void PinDefaults (const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].reg=0;
	PORT->Group[PIN_GET_PORT(pin.pinNum)].DIRCLR.reg=(1<<PIN_GET_PIN(pin.pinNum));
}

static inline void PinHigh (const pin_t pin) __attribute__((always_inline));
static inline void PinHigh (const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].OUTSET.reg=(1<<PIN_GET_PIN(pin.pinNum));
}

static inline void PinLow (const pin_t pin) __attribute__((always_inline));
static inline void PinLow (const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].OUTCLR.reg=(1<<PIN_GET_PIN(pin.pinNum));
}

static inline void PinEnableInputBuffer(const pin_t pin) __attribute__((always_inline));
static inline void PinEnableInputBuffer(const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].bit.INEN=1;
}

static inline void PinSetAsInput (const pin_t pin) __attribute__((always_inline));
static inline void PinSetAsInput (const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].DIRCLR.reg=(1<<PIN_GET_PIN(pin.pinNum));
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].bit.INEN=1;
}

static inline void PinSetAsOutput (const pin_t pin) __attribute__((always_inline));
static inline void PinSetAsOutput (const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].DIRSET.reg=(1<<PIN_GET_PIN(pin.pinNum));
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].reg=0;
}


static inline void PinSetSetMux(const pin_t pin, const pinPeripheralMux_t mux) __attribute__((always_inline));
static inline void PinSetSetMux(const pin_t pin, const pinPeripheralMux_t mux)
{
	if (PIN_GET_PIN(pin.pinNum) & 0x001)
	{
		PORT->Group[PIN_GET_PORT(pin.pinNum)].PMUX[PIN_GET_PIN(pin.pinNum)/2].bit.PMUXO=mux;
	}else
	{
		PORT->Group[PIN_GET_PORT(pin.pinNum)].PMUX[PIN_GET_PIN(pin.pinNum)/2].bit.PMUXE=mux;
	}

	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].reg|=PORT_PINCFG_PMUXEN;

}

static inline void PinSetGPIOMux(const pin_t pin) __attribute__((always_inline));
static inline void PinSetGPIOMux(const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].reg &= ~(PORT_PINCFG_PMUXEN);
}

static inline void PinSetSetInputPulledHigh(const pin_t pin) __attribute__((always_inline));
static inline void PinSetSetInputPulledHigh(const pin_t pin)
{
	PinSetAsInput(pin);
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].reg=PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	PORT->Group[PIN_GET_PORT(pin.pinNum)].OUTSET.reg=(1<<PIN_GET_PIN(pin.pinNum));
}

static inline void PinSetSetInputPulledLow(const pin_t pin) __attribute__((always_inline));
static inline void PinSetSetInputPulledLow(const pin_t pin)
{
	PinSetAsInput(pin);
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].reg=PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	PORT->Group[PIN_GET_PORT(pin.pinNum)].OUTCLR.reg=(1<<PIN_GET_PIN(pin.pinNum));
}

static inline void PinEnableStrongDrive(const pin_t pin) __attribute__((always_inline));
static inline void PinEnableStrongDrive(const pin_t pin)
{
	PORT->Group[PIN_GET_PORT(pin.pinNum)].PINCFG[PIN_GET_PIN(pin.pinNum)].bit.DRVSTR=1;
}



#endif /* DRIVERS_PIN_PIN_H_ */
