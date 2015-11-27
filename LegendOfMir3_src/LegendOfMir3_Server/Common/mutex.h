//------------------------------------------------------------------------
// »¥³âÁ¿
//
//------------------------------------------------------------------------
#ifndef MUTEX_H_
#define MUTEX_H_

#include "Common.h"

class Mutex 
{
public:
	Mutex(void)
	{
		InitializeCriticalSection(&mutex_);
	}

	virtual ~Mutex(void) 
	{ 
		DeleteCriticalSection(&mutex_);
	}	

	void lockMutex(void)
	{
		EnterCriticalSection(&mutex_);
	}

	void unlockMutex(void)
	{
		EnterCriticalSection(&mutex_);
	}

protected:
	CRITICAL_SECTION mutex_;
};

class AutoMutex
{
public:
	AutoMutex(Mutex* mutexPtr):mutexPtr_(mutexPtr)
	{
		mutexPtr_->lockMutex();
	}

	virtual ~AutoMutex(void) 
	{ 
		mutexPtr_->unlockMutex();
	}	

protected:
	Mutex* mutexPtr_;
};

#endif