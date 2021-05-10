/*
 * main.cpp
 *
 *  Created on: Jun 2, 2019
 *      Author: Trampas
 */

//all includes should be relative to the source directory.
#include "board.h"

#include "./drivers/flash/flash.h"
#include "app/commands.h"
#include "./drivers/wdt/wdt.h"
#include "app/nzs.h"
#include "drivers/USB/USBAPI.h"
#include "drivers/USB/USBCharDevice.h"

//this function sets up the pins, you can do different if you like
void configure_pins(void)
{
	PinConfig(GREEN_LED_PIN);
	PinConfig(RED_LED_PIN);
	PinConfig(PIN_CAL_BUTTON);
	RED_LED(0);
	GREEN_LED(0);
}
#define SerialUSB Serial

USBCharDevice usbSerial;
int main()
{
	NZS nzs;
	uint32_t ctrl;
	uint32_t to;
	bool led_state=false;
	
		//enable the floating point hardware unit
	ctrl=__get_CONTROL();
	__set_CONTROL(ctrl | 0x04) ;
	 SCB->CPACR |= ((3UL << 10*2) | /* set CP10 Full Access */ 
                    (3UL << 11*2) ); /* set CP11 Full Access */ 
	 
	 
	SysTickInit(NULL); //setup the systick timer to tick ever millisecond
	wdtInit(WDT_16384_CYCLES);

	
	configure_pins();  //configure GPIO pins, function above

	__enable_irq(); //enable interrupts
	
	USBDevice.init();
	USBDevice.attach();
	
	delay_ms(1000); //delay for USB start up... 
	
	//	//start up the USB serial interface
//	//TODO check for power on USB before doing this...
	SerialUSB.begin(115200);
	
	
	// configure syslog early on
//	dbgUART.setup(PIN_MCU_TXD,PIN_MCU_RXD,UART_BAUD_RATE);
	SysLogInit(&usbSerial, LOG_ALL);
	LOG("Power up, 0x%0X",RSTC->RCAUSE.reg);

	wdtClear();

	//setup other things you might need
	flashInit();
	//I2C.init(PIN_SDA,PIN_SCL,100000UL);

	//configure the command line interface
	commandsInit(&usbSerial,NULL);

	//wdtClear();

	

	nzs.begin();
	to=millis();
	while(1)
	{
		wdtClear();
		//LOG("Tick %d",millis());
		nzs.loop();
		if (millis()>(to+500))
		{
			led_state=!led_state;
			GREEN_LED(led_state);
			to=millis();
		}
	}

}


