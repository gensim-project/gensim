//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
//
// =====================================================================

// =====================================================================
// HEADERS
// =====================================================================

#include <pthread.h>

#include <cassert>
#include <cstdlib>

#include "concurrent/Mutex.h"
#include "util/LogContext.h"

UseLogContext(LogInfrastructure);
DeclareChildLogContext(LogMutex, LogInfrastructure, "Mutex");

// =====================================================================
// METHODS
// =====================================================================

namespace archsim
{
	namespace concurrent
	{

// Construct a Mutex
//
		Mutex::Mutex(bool recursive, std::string _name) : name(_name), mutex_(NULL)
		{
			bool success = true;
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(malloc(sizeof(pthread_mutex_t)));
			pthread_mutexattr_t attr;

			// Initialise Mutex attributes
			//
			success = (pthread_mutexattr_init(&attr) == 0);
			assert(success);

			// Initialize the mutex attribute type
			//
			int kind = (recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
			success = (pthread_mutexattr_settype(&attr, kind) == 0);
			assert(success);

			// FINALLY initialise mutex
			//
			success = (pthread_mutex_init(mutex, &attr) == 0);
			assert(success);

			// Destroy attributes
			//
			success = (pthread_mutexattr_destroy(&attr) == 0);
			assert(success);

			(void)success;

			// Assign fully instantiated Mutex to mutex_ attribute
			//
			mutex_ = mutex;
#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Created Mutex";
#endif
		}

// Destruct Mutex
//
		Mutex::~Mutex()
		{
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);
			assert(mutex != NULL);
			pthread_mutex_destroy(mutex);
			free(mutex);
#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Destroyed Mutex";
#endif
		}

// Acquire Mutex
//
		bool Mutex::acquire()
		{
			bool success = true;
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);

			success = (pthread_mutex_lock(mutex) == 0);
#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Acquired Mutex in thread " << pthread_self();
#endif

			return success;
		}

// Acquire Mutex with a timeout. Returns true if the mutex was acquired
		bool Mutex::acquire_timeout(int millis)
		{
			bool success = true;
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);

			timespec t;
			t.tv_nsec = millis * 1000000;
			t.tv_sec = 0;

#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Acquiring Mutex in thread " << pthread_self();
#endif

			success = (pthread_mutex_timedlock(mutex, &t) == 0);

#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Acquiring Mutex in thread " << pthread_self() << (success ? " succeeded" : " timed out");
#endif

			return success;
		}

// Release Mutex
//
		bool Mutex::release()
		{
			bool success = true;
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);

			success = (pthread_mutex_unlock(mutex) == 0);

#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Released Mutex in thread " << pthread_self();
#endif

			return success;
		}

// Try to acquire Mutex
//
		bool Mutex::tryacquire()
		{
			bool success = true;
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);

			success = (pthread_mutex_trylock(mutex) == 0);

#if DEBUG_MUTEX
			LC_DEBUG1(LogMutex) << std::hex << this << " (" << name << ") Try-Acquired Mutex in thread " << pthread_self() << ": " << (success ? "succeeded" : "failed");
#endif

			return success;
		}
	}
}  // archsim::concurrent
