/*
 * TimerManager.cpp
 *
 *  Created on: 17 Dec 2013
 *      Author: harry
 */

#include "util/LogContext.h"
#include "util/TimerManager.h"
#include "concurrent/ConditionVariable.h"

#include <cassert>
#include <chrono>
#include <vector>

DeclareLogContext(LogTimers, "Timers");

namespace archsim
{
	namespace util
	{
		namespace timing
		{
			TimerSource::~TimerSource() {}

			bool TimerSource::Interrupt()
			{
				return false;
			}

			TimerManager::TimerManager(TimerSource *source) : Thread("Timer Manager"), timer_source(source), next_ctx(0), timers_mutex(true), terminate(false)
			{

			}

			TimerManager::~TimerManager()
			{
				LC_DEBUG1(LogTimers) << "Stopping timer manager...";
				terminate = true;

				timers_mutex.acquire();
				timers_cond.signal();
				timers_mutex.release();

				join();
				LC_DEBUG1(LogTimers) << "Timers stopped";
				delete timer_source;
			}

			TimerCtx TimerManager::RegisterOneshotInternal(mgr_duration_t duration, timer_callback_t callback, void* data)
			{
				TimerState *new_timer = new TimerState();
				new_timer->callback = callback;
				new_timer->data = data;

				timers_mutex.acquire();
				new_timer->issue_time = TimerState::timer_t::now();
				new_timer->invoke_time = new_timer->issue_time + duration;
				timers.push_back(new_timer);
				timers_mutex.release();
				timers_cond.signal();

				//If we're currently waiting on a timer, interrupt it
				timer_source->Interrupt();

				return new_timer;
			}

			void TimerManager::ResetTimer(TimerCtx timer)
			{
				timer->invoke_time = TimerState::timer_t::now() + (timer->invoke_time - timer->issue_time);
			}

			void TimerManager::DeregisterTimer(TimerCtx timer)
			{
				//timer with null callback will be deregistered the next time we loop around
				timer->callback = NULL;
			}

			void TimerManager::run()
			{
				terminate = false;

				// Acquire the mutex
				timers_mutex.acquire();

				// Loop until told otherwise
				while (!terminate) {
					// Wait for a timer to become available
					while (!timers.size() && !terminate) {
						// Release mutex and wait.
						timers_cond.wait(timers_mutex);
					}

					// If we're terminating at this point, bail out quickly.
					if (terminate) {
						break;
					}

					// We have the mutex
					TimerState* next_timer = NULL;
					std::vector<TimerState*> removed_timers;

					for (timer_vector_t::iterator i = timers.begin(); i != timers.end(); ++i) {
						TimerState* timer = *i;

						if (timer->callback == NULL) {
							removed_timers.push_back(timer);
							continue;
						}

						if (!next_timer || timer->get_ticks_left() < next_timer->get_ticks_left()) next_timer = timer;
					}

					for (auto timer : removed_timers) timers.remove(timer);

					// If we had some timers, but they've been removed, just continue and wait until we get a new one
					if (!next_timer) {
						continue;
					}

					// Release the mutex
					timers_mutex.release();

					// Wait for the timer to expire
					mgr_duration_t wait_ticks = std::chrono::duration_cast<mgr_duration_t>(next_timer->get_ticks_left());

					LC_DEBUG3(LogTimers) << "Got a timer and waiting " << wait_ticks.count() << " ticks for it to expire";

					wait_ticks = std::chrono::duration_cast<mgr_duration_t>(timer_source->WaitN(wait_ticks));
					LC_DEBUG3(LogTimers) << "Waited " << wait_ticks.count() << " ticks.";

					std::vector<TimerState*> elapsed_timers;

					// Reacquire the mutex.
					timers_mutex.acquire();

					// Now loop through the list again, updating the timers and calling any which have expired
					for(timer_vector_t::iterator i = timers.begin(); i != timers.end(); ++i) {
						TimerState *timer = *i;
						if(TimerState::timer_t::now() > timer->invoke_time) {
							elapsed_timers.push_back(timer);
						}
					}

					for (auto timer : elapsed_timers) {
						timers.remove(timer);
						timer->invoke_time = TimerState::timer_t::now();
						if (timer->callback) timer->callback(timer, timer->data);
						delete timer;
					}
				}

				timers_mutex.release();
			}

		}
	}
}
