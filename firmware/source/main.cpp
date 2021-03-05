/*
 * main.cpp
 *
 *  Created on: Jun 2, 2019
 *      Author: Trampas
 */

//all includes should be relative to the source directory.
#include "board.h"
#include "drivers/I2C/I2CMaster.h"
#include "drivers/UART/UART.h"
#include "drivers/adc/adc.h"
#include "libraries/libc/vsnprintf.h"
#include "./drivers/flash/flash.h"
#include "commands.h"



UART dbgUART;
I2CMaster I2C;


//this function sets up the pins, you can do different if you like
void configure_pins(void)
{
	PinConfig(PWR_CNTRL);
	PinHigh(PWR_CNTRL);

	PinConfig(GREEN_LED_PIN);
	PinConfig(RED_LED_PIN);
}


int main()
{
	uint32_t ctrl;

	SysTickInit(NULL); //setup the systick timer to tick ever millisecond
	WATCHDOG_ENABLE();

	configure_pins();  //configure GPIO pins, function above

	// configure syslog early on
	dbgUART.setup(PIN_MCU_TXD,PIN_MCU_RXD,UART_BAUD_RATE);
	SysLogInit(&dbgUART, LOG_ALL);
	LOG("Power up");

	//enable the floating point hardware unit
	ctrl=__get_CONTROL();
	__set_CONTROL(ctrl | 0x04) ;

	WATCHDOG_KICK();

	//setup other things you might need
	flashInit();
	I2C.init(PIN_SDA,PIN_SCL,100000UL);

	//configure the command line interface
	commandsInit(&dbgUART);

	WATCHDOG_KICK();

	__enable_irq(); //enable interrupts

	while(1)
	{
		WATCHDOG_KICK();
		commandsProcess(); //process command line interface
	}

}


