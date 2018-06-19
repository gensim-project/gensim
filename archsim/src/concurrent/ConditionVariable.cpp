/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include <pthread.h>

#include <cassert>
#include <stdlib.h>

#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"

#include "util/LogContext.h"

UseLogContext(LogInfrastructure);
DeclareChildLogContext(LogCondVar, LogInfrastructure, "CondVar");

namespace archsim
{
	namespace concurrent
	{

// Construct a ConditionVariable
//
		ConditionVariable::ConditionVariable(std::string _name) : name(_name), condvar_(NULL)
		{
			bool success = true;
			pthread_cond_t* condvar = static_cast<pthread_cond_t*>(malloc(sizeof(pthread_cond_t)));
			pthread_condattr_t attr;

			// Initialise ConditionVariable attributes
			//
			success = (pthread_condattr_init(&attr) == 0);
			assert(success);

			// FINALLY initialise ConditionVariable
			//
			success = (pthread_cond_init(condvar, &attr) == 0);
			assert(success);

			// Destroy attributes
			//
			success = (pthread_condattr_destroy(&attr) == 0);
			assert(success);

			// Assign fully instantiated ConditionVariable to condvar_ attribute
			//
			condvar_ = condvar;

			// Use the success variable in case we have debugging disabled
			(void)success;

#if DEBUG_MUTEX
			LOG(LOG_DEBUG4) << "[CONDVAR] " << std::hex << this << " (" << name << ") Created ConditionVar";
#endif
		}

// Destruct ConditionVariable
//
		ConditionVariable::~ConditionVariable()
		{
			pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);
			assert(condvar != NULL);
			pthread_cond_destroy(condvar);
			free(condvar);

#if DEBUG_MUTEX
			LOG(LOG_DEBUG4) << "[CONDVAR] " << std::hex << this << " (" << name << ") Destroyed ConditionVar";
#endif
		}

// Unblocks a thread waiting for this condition variable.
//
		bool ConditionVariable::signal()
		{
			bool success = true;
			pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);

			success = (pthread_cond_signal(condvar) == 0);

#if DEBUG_MUTEX
			LOG(LOG_DEBUG4) << "[CONDVAR] " << std::hex << this << " (" << name << ") Signal ConditionVar in thread " << pthread_self();
			;
#endif

			return success;
		}

// Unblocks all threads waiting for this condition variable.
//
		bool ConditionVariable::broadcast()
		{
			bool success = true;
			pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);

			success = (pthread_cond_broadcast(condvar) == 0);

#if DEBUG_MUTEX
			LOG(LOG_DEBUG4) << "[CONDVAR] " << std::hex << this << " (" << name << ") Broadcast ConditionVar in thread " << pthread_self();
			;
#endif

			return success;
		}

// Wait on this condition variable.
//
		bool ConditionVariable::wait(const archsim::concurrent::Mutex& m)
		{
			bool success = true;
			pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);
			pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(m.mutex_);

			success = (pthread_cond_wait(condvar, mutex) == 0);

#if DEBUG_MUTEX
			LOG(LOG_DEBUG4) << "[CONDVAR] " << std::hex << this << " (" << name << ") Wait ConditionVar in thread " << pthread_self();
			;
#endif

			return success;
		}
	}
}  // archsim::concurrent
