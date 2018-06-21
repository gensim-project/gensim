/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

// =====================================================================
//
// Description:
//
// Thread class that you should derive from.
//
// =====================================================================

#ifndef INC_CONCURRENT_THREAD_H_
#define INC_CONCURRENT_THREAD_H_

#include "define.h"

#include <string>

namespace archsim
{
	namespace concurrent
	{

// -----------------------------------------------------------------------
// A ThreadHandle is used to refer to to a specific thread and test for
// equality.
//
		class ThreadHandle
		{
		public:
			enum Kind {
				SELF,
				INVALID
			};

			explicit ThreadHandle(Kind k);

			~ThreadHandle();

			// Test if this ThreadHandle is valid
			// NOTE: The 'const' keyword at the end tells others that this method
			//       won't change the logical state of this object.
			bool is_valid() const;

			// Test if thread is running.
			//
			bool is_equal() const;

			// Initialise handle
			//
			void init(Kind k);

			// Thread data - this stuff is platform specific. At the moment we we
			// we use pthreads but this might be different on another platform.
			//
			class ThreadHandleData;  // Nested class (forward declaration only)

			ThreadHandleData* get_thread_handle_data()
			{
				return data_;
			};

		private:
			// Thread platform specific data.
			//
			ThreadHandleData* data_;
		};

// -----------------------------------------------------------------------
// If you want to create and run threads, derive your class from the Thread
// class. When the start() method is called the new thread starts running
// the run() method.
//
		class Thread : public ThreadHandle
		{
		public:
			Thread(std::string threadname = "(anonymous)");
			virtual ~Thread();

			// Start thread by calling run() method in the new thread.
			//
			void start();

			// Wait until thread terminates.
			//
			void join();

			// Immediately kill the thread.
			//
			void kill();

			// Abstract run() method that YOU need to implement. This is the method
			// that get's executed in the newly created thread by start().
			//
			virtual void run() = 0;

		private:
			std::string name;
		};

		class LoopThread : public Thread
		{
		public:
			LoopThread(std::string name = "(anonymous loop)");
			virtual ~LoopThread();

			void stop();
			void interrupt();

			virtual void run() override;

		protected:
			inline bool should_terminate() const
			{
				return terminate;
			}
			virtual void tick() = 0;

		private:
			volatile bool terminate;
		};
	}
}  // namespace archsim::concurrent

#endif  // INC_CONCURRENT_THREAD_H_
