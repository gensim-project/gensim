//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
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
	}
}  // archsim::util

#endif  // INC_UTIL_COUNTERTIMER_H_
