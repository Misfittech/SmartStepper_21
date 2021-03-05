/*
 * mutexNVIC.h
 *
 *  Created on: Jan 6, 2018
 *      Author: tstern
 */

#ifndef DRFL_LIBRARIES_MUTEX_MUTEXINTERRUPTENABLE_H_
#define DRFL_LIBRARIES_MUTEX_MUTEXINTERRUPTENABLE_H_

#include "mutex.h"


/** example code
 *
 *  void UART0_TX_ISR()
 *  {
 *
 *  }
 *
 *  int foo(void)
 *  {
 *  	MutexInterruptEnable<uint32_t,UART0->INTENABE, UART_TX_IE> Uart0TxMutex
 *  	lock_guard<MutexInterruptEnable> lock(Uart0TxMutex);
 *  	//UART0_TX_ISR() will not be called until
 *  	// the lock above goes out of scope.
 *  }
 *
 */


//this class uses a interrupt enable as mutex
// if the interrupt enable is low the code can proceed
// if it is high we will clear it then code can proceed
// when mutex is released the last release will reenable the interrupt
// if previously enabled.
template <class T, volatile T *regAddress, T bitMask, volatile T *regClrAddress = NULL>
class MutexInterruptEnable : public Mutex {
public:
	MutexInterruptEnable();
	bool try_lock(void);
	void unlock(void);
private:
	uint32_t _count;
};

template <class T, volatile T *regAddress, T bitMask, volatile T *regClrAddress = NULL>
bool MutexInterruptEnable<T,regAddress,bitMask,regClrAddress> ::MutexInterruptEnable()
{
	_count=0;
	_state=false;
}
template <class T, volatile T *regAddress, T bitMask, volatile T *regClrAddress = NULL>
bool MutexInterruptEnable<T,regAddress,bitMask,regClrAddress> ::try_lock(void)
{
	if (_count>0)
	{
		_count++;
		return true;
	}

	if (regAddress == NULL)
	{
		ERROR("need to set register address");
		return false;
	}
	if (*regAddress & bitMask)
	{
		_state=true;
	}else
	{
		_state=false;
	}
	if (regClrAddress == NULL)
	{
		*regAddress =  *regAddress & (~bitMask); //clear the bit
	}else
	{
		*regClrAddress=bitMask;
	}
	_count=1;

	return true;
}

template <class T, volatile T *regAddress, T bitMask, volatile T *regClrAddress = NULL>
void MutexInterruptEnable<T,regAddress,bitMask,regClrAddress> ::unlock(void)
{
	if (_count>0)
	{
		_count--;
		if (_count == 0 && _state==true)
		{
			if (regClrAddress == NULL)
			{
				*regAddress =  *regAddress | (bitMask); //set the bit
			} else
			{
				//we have a set and clear registers
				*regAddress = bitMask;
			}
			_count=1;
		}
	}
}



#endif /* DRFL_LIBRARIES_MUTEX_MUTEXINTERRUPTENABLE_H_ */
