//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: Declaration of a ConditionVariable class.
//
// =====================================================================

#ifndef INC_CONCURRENT_CONDITIONVARIABLE_H_
#define INC_CONCURRENT_CONDITIONVARIABLE_H_

#include <string>

namespace archsim
{
	namespace concurrent
	{

// -------------------------------------------------------------------------
// FORWARD DECLARATIONS
//
		class Mutex;

// -------------------------------------------------------------------------
// ConditionVariable class
//
		class ConditionVariable
		{
		public:
			std::string name;

			// Initializes the condition variable
			//
			ConditionVariable(std::string _name = "");

			// Releases and removes the condition variable
			//
			~ConditionVariable();

		public:
			// Unblocks a thread waiting for this condition variable.
			//
			bool signal();

			// Unblock all threads waiting for this condition variable.
			//
			bool broadcast();

			// Wait on this condition variable.
			//
			bool wait(const Mutex& m);

		private:
			void* condvar_;

		private:
			ConditionVariable(const ConditionVariable& cv);
			void operator=(const ConditionVariable&);
		};
	}
}  // namespace archsim::concurrent

#endif  // INC_CONCURRENT_CONDITIONVARIABLE_H_
