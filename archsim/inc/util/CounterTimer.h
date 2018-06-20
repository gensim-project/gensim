/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

// =====================================================================
//
// Description: Portable time recording class.
//
// CounterTimer t("timer");
//
//  t.start();
//  while(condition) {
//    ... // do some operations
//  }
//  t.stop();
//
//  ... // do some operation that does not account to timing
//
//  t.start();
//  while(condition) {
//    ... // do some OTHER operations
//  }
//  t.stop();
//
//  printf("Elapsed time: %0.2f s", t.get_elapsed_seconds());
//
// =====================================================================

#ifndef INC_UTIL_COUNTERTIMER_H_
#define INC_UTIL_COUNTERTIMER_H_

#include "define.h"

#include <chrono>

namespace archsim
{
	namespace util
	{
		class CounterTimer
		{
		public:
			typedef double seconds_t;
			typedef uint64_t nanoseconds_t;

			explicit CounterTimer() : elapsed(0) { }

			// Start counting time
			inline void Start()
			{
				start = std::chrono::system_clock::now();
			}

			// Stop counting time
			inline void Stop()
			{
				std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
				std::chrono::duration<double> span = stop - start;

				elapsed += span.count();
			}

			// Reset CounterTimer to default values
			inline void Reset()
			{
				elapsed = 0;
			}

			// Retrieve elapsed time in seconds
			inline seconds_t GetElapsedS() const
			{
				return elapsed;
			}

		private:
			std::chrono::system_clock::time_point start;
			seconds_t elapsed;
		};

		class CounterTimerContext
		{
		public:
			CounterTimerContext(CounterTimer &counter) : counter_(counter)
			{
				counter_.Start();
			}
			~CounterTimerContext()
			{
				counter_.Stop();
			}

		private:
			CounterTimer &counter_;
		};
	}
}  // archsim::util

#endif  // INC_UTIL_COUNTERTIMER_H_
