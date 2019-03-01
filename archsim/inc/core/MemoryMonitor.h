/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "core/thread/ThreadInstance.h"

#include <map>
#include <mutex>

namespace archsim
{
	namespace core
	{

		/*
		 * Class responsible for providing LL/SC semantics on memory interfaces.
		 */
		class MemoryMonitor
		{
		public:
			virtual ~MemoryMonitor() = default;

			// Inform the monitor that given thread wants to acquire a monitor on the given address
			virtual void AcquireMonitor(archsim::core::thread::ThreadInstance *thread, Address addr) = 0;

			// If the given thread has a lock on the given address, lock the monitor to allow the thread to safely access memory at the given address
			virtual bool LockMonitor(archsim::core::thread::ThreadInstance *thread, Address addr) = 0;

			// Inform the monitor that the given thread has finished safely accessing the given address
			virtual void UnlockMonitor(archsim::core::thread::ThreadInstance *thread, Address addr) = 0;

			// Notify the monitor that a write has been performed by the given thread to the given address
			virtual void Notify(archsim::core::thread::ThreadInstance *thread, Address addr) = 0;

			virtual void Lock() = 0;
			virtual void Unlock() = 0;
		};

		class BaseMemoryMonitor : public MemoryMonitor
		{
		public:
			virtual void AcquireMonitor(archsim::core::thread::ThreadInstance *thread, Address addr);
			virtual bool LockMonitor(archsim::core::thread::ThreadInstance *thread, Address addr);
			virtual void UnlockMonitor(archsim::core::thread::ThreadInstance *thread, Address addr);
			virtual void Notify(archsim::core::thread::ThreadInstance *thread, Address addr);

			virtual void Lock()
			{
				lock_.lock();
			}
			virtual void Unlock()
			{
				lock_.unlock();
			}

		private:
			// Map of thread ID to monitored address
			std::map<int, Address> monitors_;
			std::recursive_mutex lock_;
		};

	}
}