/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LWLock.h
 * Author: harry
 *
 * Created on 13 August 2018, 09:33
 */

#ifndef LWLOCK_H
#define LWLOCK_H

#include <atomic>

namespace archsim
{
	namespace concurrent
	{
		class LWLock
		{
		public:
			LWLock() : locked_(false) {}

			void lock()
			{
				bool expected = false;
				while(!locked_.compare_exchange_strong(expected, true)) {
					asm volatile("pause");
				}
			}

			void unlock()
			{
				locked_ = false;
			}

		private:
			std::atomic<bool> locked_;
		};
	}
}

#endif /* LWLOCK_H */

