/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * TimerManager.h
 *
 *  Created on: 17 Dec 2013
 *      Author: harry
 */

#ifndef TIMERMANAGER_H_
#define TIMERMANAGER_H_

#include "concurrent/Thread.h"
#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"

#include <chrono>
#include <cstdint>
#include <list>
#include <unistd.h>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace util
	{
		namespace timing
		{

			typedef std::chrono::high_resolution_clock timing_clock_t;

			class TimerSource
			{
			public:
				virtual ~TimerSource();

				typedef timing_clock_t::duration rt_duration_t;

				template <typename T> inline rt_duration_t WaitN(T duration)
				{
					return Wait(std::chrono::duration_cast<rt_duration_t>(duration));
				}
				virtual rt_duration_t Wait(rt_duration_t ticks) = 0;
				virtual bool Interrupt();
			};

			template<typename clock_t=std::chrono::high_resolution_clock, typename tick_t = std::chrono::milliseconds, bool busy=false> class RealTimeTimerSource : public TimerSource
			{
			public:
				rt_duration_t Wait(rt_duration_t ticks)
				{
					Interrupted = false;
					typename clock_t::time_point pre_time = clock_t::now();
					typename clock_t::time_point post_time = pre_time + ticks;
					while(!Interrupted && clock_t::now() < post_time) if(!busy)usleep(1000);
					return (std::chrono::duration_cast<tick_t>(clock_t::now() - pre_time));
				}

				bool Interrupt()
				{
					Interrupted = true;
					return true;
				}

			private:
				bool Interrupted;
			};

			class InstructionTimerSource : public TimerSource
			{
			public:
				InstructionTimerSource(const gensim::Processor &processor);

				uint32_t WaitTicks(uint32_t ticks);

			private:
				const gensim::Processor &cpu;
			};

			class TimerState;

			typedef TimerState* TimerCtx;
			typedef void (*timer_callback_t)(TimerCtx, void*);

			class TimerState
			{
			public:
				typedef timing_clock_t timer_t;
				typedef timer_t::duration time_difference_t;
				typedef timer_t::time_point time_point_t;

				timer_callback_t callback;

				time_point_t issue_time;
				time_point_t invoke_time;

				void* data;

				inline time_difference_t get_ticks_left() const
				{
					return invoke_time - timer_t::now();
				}

				inline time_difference_t get_delta() const
				{
					return invoke_time - issue_time;
				}
			};


		}
	}
}

namespace archsim
{
	namespace util
	{
		namespace timing
		{

// Intelligent timer manager. Allows the registration of timer callbacks, callable after the given time.
			class TimerManager : public concurrent::Thread
			{
			public:
				TimerManager(TimerSource *source);
				~TimerManager();

				typedef timing_clock_t::duration mgr_duration_t;
				template <typename T> TimerCtx RegisterOneshot(T duration, timer_callback_t callback, void* data)
				{
					return RegisterOneshotInternal(std::chrono::duration_cast<mgr_duration_t>(duration), callback, data);
				}

				void ResetTimer(TimerCtx timer);
				void DeregisterTimer(TimerCtx timer);

				void run();

			private:
				TimerCtx RegisterOneshotInternal(mgr_duration_t duration, timer_callback_t callback, void* data);

				TimerSource *timer_source;
				uint32_t next_ctx;

				//Mutex used to lock the timers vector while adding new timers
				concurrent::Mutex timers_mutex;
				concurrent::ConditionVariable timers_cond;
				typedef std::list<TimerState*> timer_vector_t;
				timer_vector_t timers;

				volatile bool terminate;

			};

		}
	}
}


#endif /* TIMERMANAGER_H_ */
