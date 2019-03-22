/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/MemoryMonitor.h"

DeclareLogContext(LogMonitor, "Monitor");

using namespace archsim::core;

void BaseMemoryMonitor::Lock(archsim::core::thread::ThreadInstance* thread)
{
	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << ": locking monitor";

	if (thread != locked_thread_) {
		lock_.lock();
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << ": locked monitor";
		locked_thread_ = thread;
	} else {
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << ": already had monitor lock";
	}
}

void BaseMemoryMonitor::Unlock(archsim::core::thread::ThreadInstance* thread)
{
	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << ": unlocking monitor";

	if(locked_thread_ == thread) {
		locked_thread_ = nullptr;
		lock_.unlock();
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << ": unlocked monitor";
	} else {
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << ": did not have monitor lock";
	}
}

void BaseMemoryMonitor::AcquireMonitor(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	Lock(thread);

	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " took monitor on " << addr;
	monitors_[thread->GetThreadID()] = addr;

	Unlock(thread);
}

bool BaseMemoryMonitor::LockMonitor(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	Lock(thread);

	if(monitors_.count(thread->GetThreadID()) != 0 && monitors_.at(thread->GetThreadID()) == addr) {
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " successfully locked on " << addr;
		return true;
	} else {
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " failed to lock on " << addr;
		Unlock(thread);
		return false;
	}
}

void BaseMemoryMonitor::UnlockMonitor(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " unlocking monitor";
	Unlock(thread);
}

void BaseMemoryMonitor::Notify(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	Lock(thread);
	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " notified " << addr;
	for(const auto &monitor : monitors_) {
		if(monitor.first != thread->GetThreadID() && addr == monitor.second) {
			monitors_.erase(monitor.first);
		}
	}

	Unlock(thread);
}
