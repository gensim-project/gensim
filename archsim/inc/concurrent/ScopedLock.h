//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
// ScopedLock should be used to ensure that a Mutex is locked upon
// construction and released upon destruction. Declaring a ScopedLock
// as the first statement in method/function gives it similar
// semantics as using the synchronised keyword for a JAVA method.
//
//
// The intention is to instantiate one of these on the stack at the top
// of some scope to be assured that C++ destruction of the object will
// always release the Mutex and thus avoid  a host of nasty
// multi-threading problems in the face of exceptions, etc.
// This design pattern is called RAII:
//  @http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization
//
// =====================================================================

#ifndef INC_CONCURRENT_SCOPEDLOCK_H_
#define INC_CONCURRENT_SCOPEDLOCK_H_

#include "concurrent/Mutex.h"

namespace archsim
{
	namespace concurrent
	{

// -------------------------------------------------------------------------
// ScopedLock class
//
		class ScopedLock
		{
		private:
			Mutex &M;
			ScopedLock(const ScopedLock &);      // DO NOT COPY
			void operator=(const ScopedLock &);  // DO NOT ASSIGN

		public:
			explicit ScopedLock(Mutex &m) : M(m)
			{
				M.acquire();
			}
			~ScopedLock()
			{
				M.release();
			}

			// holds - Returns true if this locker instance holds the specified lock
			//
			bool holds(const Mutex &lock) const
			{
				return &M == &lock;
			}
		};
	}
}  // namespace archsim::util::system

#endif  // INC_CONCURRENT_SCOPEDLOCK_H_
