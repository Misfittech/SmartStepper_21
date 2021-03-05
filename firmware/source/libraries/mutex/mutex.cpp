/*
 * mutex.cpp
 *
 *  Created on: Jan 6, 2018
 *      Author: tstern
 */

#include "mutex.h"

#include "libraries/syslog/syslog.h"
#include "drivers/system/system.h"

Mutex::Mutex()
{
	_state=false;
};


void Mutex::lock(void)
{
	while(!try_lock())
	{

	}
}

bool Mutex::try_lock(void)
{
	bool interruptState;
	bool ret=false;;
	interruptState=InterruptDisable(); //disable global interrupts
	if (_state == false)
	{
		_state= true;
		ret=true; //you get the mutex
	}
	InterruptEnable(interruptState);
	return ret;
}

void Mutex::unlock(void)
{
	bool interruptState;

	interruptState=InterruptDisable(); //disable global interrupts
	if (_state == true)
	{
		_state=false;
	}
	InterruptEnable(interruptState);
}
