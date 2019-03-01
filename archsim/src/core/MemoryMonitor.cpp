/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/MemoryMonitor.h"

DeclareLogContext(LogMonitor, "Monitor");

using namespace archsim::core;

void BaseMemoryMonitor::AcquireMonitor(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	Lock();

	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " took monitor on " << addr;
	monitors_[thread->GetThreadID()] = addr;

	Unlock();
}

bool BaseMemoryMonitor::LockMonitor(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	Lock();

	if(monitors_.count(thread->GetThreadID()) != 0 && monitors_.at(thread->GetThreadID()) == addr) {
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " successfully locked on " << addr;
		return true;
	} else {
		LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " failed to lock on " << addr;
		Unlock();
		return false;
	}
}

void BaseMemoryMonitor::UnlockMonitor(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " unlocked on " << addr;
	Unlock();
}

void BaseMemoryMonitor::Notify(archsim::core::thread::ThreadInstance* thread, Address addr)
{
	Lock();
	LC_DEBUG1(LogMonitor) << "Thread " << thread->GetThreadID() << " notified " << addr;
	for(const auto &monitor : monitors_) {
		if(monitor.first != thread->GetThreadID() && addr == monitor.second) {
			monitors_.erase(monitor.first);
		}
	}
	Unlock();
}
