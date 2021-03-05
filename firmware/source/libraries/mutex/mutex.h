/*
 * mutex.h
 *
 *  Created on: Jan 6, 2018
 *      Author: tstern
 */

#ifndef DRFL_LIBRARIES_MUTEX_MUTEX_H_
#define DRFL_LIBRARIES_MUTEX_MUTEX_H_

//#include "libraries/syslog/syslog.h"

class Mutex {
public:
	Mutex();
	virtual void lock(void);
	virtual bool try_lock(void);
	virtual void unlock(void);
private:
	bool _state;
};

template <class T>
class lock_guard {
public:
	lock_guard(T &mutex) {
		_ptrMutex = &mutex;
		mutex.lock();
	}
	~lock_guard()
	{
		_ptrMutex->unlock();
	}
private:
	T * _ptrMutex;
};



#endif /* DRFL_LIBRARIES_MUTEX_MUTEX_H_ */
