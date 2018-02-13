//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  Multicore timing model interface.
//
// =====================================================================

#ifndef _CoherencyThreadsafeQueue_h_
#define _CoherencyThreadsafeQueue_h_

// =====================================================================
// HEADERS
// =====================================================================
#include <stdint.h>
#ifdef CYCLE_ACC_SIM

#if 0
inline void __sync_synchronize() {};
#else

#if __GNUC__ == 4
# if __GNUC_MINOR__ == 8
#  if __GNUC_PATCHLEVEL__ == 1
#   include <c++/4.8.1/ext/atomicity.h>
#  elif __GNUC_PATCHLEVEL__ == 2
#   include <c++/4.8.2/ext/atomicity.h>
#  else
#   error "Unsupported GCC revision"
#  endif
# elif __GNUC_MINOR__ == 7
#  include <c++/4.7.2/ext/atomicity.h>
# else
#  error "Unsupported GCC minor"
# endif
#else
# error "Unsupported GCC major"
#endif

#endif
#endif  // CYCLE_ACC_SIM
#include <string.h>

#include <ctime>
#include <sys/time.h>
#include <pthread.h>
#if (defined(__APPLE__) && defined(__MACH__))
#define pthread_yield() pthread_yield_np()
#endif
// =====================================================================
// FORWARD DECLARATIONS
// =====================================================================

#define _timersub(a, b, result)                               \
	do                                                    \
	{                                                     \
		(result).tv_sec = (a).tv_sec - (b).tv_sec;    \
		(result).tv_usec = (a).tv_usec - (b).tv_usec; \
		if ((result).tv_usec < 0)                     \
		{                                             \
			--(result).tv_sec;                    \
			(result).tv_usec += 1000000;          \
		}                                             \
	} while (0)

#define _timeradd(a, b, result)                               \
	do                                                    \
	{                                                     \
		(result).tv_sec = (a).tv_sec + (b).tv_sec;    \
		(result).tv_usec = (a).tv_usec + (b).tv_usec; \
		if ((result).tv_usec >= 1000000)              \
		{                                             \
			(result).tv_sec++;                    \
			(result).tv_usec -= 1000000;          \
		}                                             \
	} while (0)

// =====================================================================
// CLASSES
// =====================================================================

// -----------------------------------------------------------------------------
// ThreadsafeQueue class
//
namespace archsim
{
	namespace concurrent
	{

		template <class T, uint32_t size = (8192 * 128), bool sync = true>
		class ThreadsafeQueue
		{
		private:
			T buffer[size] __attribute__((aligned(64)));
			volatile uint32_t head;
			volatile uint32_t tail;

			// ThreadsafeQueue(ThreadsafeQueue&){};
		public:
			static const uint32_t buffer_max = size;  // 512;
			struct timeval tv_full;
			struct timeval tv_empty;
			volatile bool blocking;
			//  ThreadsafeQueue();
			//  ~ThreadsafeQueue();

			// Constructor
			//
			ThreadsafeQueue()
			//:multiplier(1ll)
			{
				tail = 0;
				head = 0;
				if (sync) blocking = false;
				tv_empty.tv_sec = 0;
				tv_empty.tv_usec = 0;
				tv_full.tv_sec = 0;
				tv_full.tv_usec = 0;
			}

			// Destructor
			//
			~ThreadsafeQueue() {}

			inline T& front()
			{
				return buffer[tail];
			}

			inline bool empty()
			{
				return (tail == head);
			}
			inline bool full()
			{
				return (((head + 1) % buffer_max) == tail);
			}

			inline uint32_t get_size()
			{
				return (((int)head - (int)tail) >= 0) ? (head - tail) : ((int)size) + ((int)head - (int)tail);
			}

			inline void pop()
			{
				uint32_t new_tail = tail;
				// while(new_tail==head){} //lets just make this incorrect useage, you should check for empty().
				new_tail = (new_tail + 1) % buffer_max;

				tail = new_tail;
				if (sync) __sync_synchronize();
			}

			bool nth_item(uint32_t n, T** item)
			{
				(*item) = &buffer[(tail + n) % buffer_max];
				return (head != ((tail + n) % buffer_max));
			}

			inline void push(T& in)
			{
				uint32_t new_head = head;
				new_head = (new_head + 1) % buffer_max;
				if (new_head == tail) {
					while (new_head == tail) {
						pthread_yield();
					}
				}
				buffer[head] = in;
				if (sync) __sync_synchronize();
				head = new_head;
				if (sync) __sync_synchronize();
			}

			inline void push_wait(T& in)
			{
				uint32_t new_head = head;
				new_head = (new_head + 1) % buffer_max;

				if (new_head == tail) {
					while (new_head == tail) {
						pthread_yield();
					}
				}

				buffer[head] = in;
				if (sync) {
					__sync_synchronize();
					blocking = true;
					__sync_synchronize();
				}
				head = new_head;
				if (sync)
					while (!empty()) {
						__sync_synchronize();
					} else
					while (!empty())
						;
				if (sync) blocking = false;
			}

			T* get_buffer()
			{
				return buffer;
			}
			volatile uint32_t* get_head()
			{
				return &head;
			}
			volatile uint32_t* get_tail()
			{
				return &tail;
			}
		};
	}
}

#endif /* _SingleDirectoryBinaryTree_h_ */
