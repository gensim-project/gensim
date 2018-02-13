/*
 * Barrier.h
 *
 *  Created on: 16 Jan 2015
 *      Author: harry
 */

#ifndef BARRIER_H_
#define BARRIER_H_

#include <cassert>

#include <atomic>

namespace archsim
{
	namespace concurrent
	{

		class LWBarrier2
		{
		public:

			LWBarrier2() : _b {0,0}, _ctr(0), _interrupt(0) {}

			inline void Wait(uint32_t my_tid)
			{
				assert(my_tid == 0 || my_tid == 1);

				int ctr = _ctr;
				int val = !ctr;

				_b[my_tid] = val;
				while(_b[!my_tid] != val && !_interrupt) asm("pause");

				_ctr = !ctr;

			}

			inline void Interrupt()
			{
				_interrupt = true;
			}

		private:
			uint32_t _ctr;
			volatile bool _b[2];
			volatile bool _interrupt;
		};

	}
}


#endif /* BARRIER_H_ */
