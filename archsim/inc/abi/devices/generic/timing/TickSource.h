/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * TickSource.h
 *
 *  Created on: 19 Jan 2015
 *      Author: harry
 */

#ifndef TICKSOURCE_H_
#define TICKSOURCE_H_

#include "concurrent/Thread.h"
#include "util/PubSubSync.h"

#include <cstdint>
#include <unordered_set>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace timing
			{
				class TickConsumer;

				class TickSource
				{
				public:
					TickSource();
					virtual ~TickSource();

					void AddConsumer(TickConsumer &consumer);
					void RemoveConsumer(TickConsumer &consumer);

					inline uint64_t GetCounter()
					{
						return tick_count_;
					}

				protected:
					void Tick(uint32_t tick_periods);
				private:
					uint64_t tick_count_;
					float microticks_;
					float microtick_scale_;

				private:
					std::unordered_set<TickConsumer*> consumers;
				};

				class MicrosecondTickSource : public archsim::concurrent::LoopThread, public TickSource
				{
				public:
					MicrosecondTickSource(uint32_t useconds);
					~MicrosecondTickSource() override;

				protected:
					void tick() override;

				private:
					uint32_t ticks;
					uint32_t recalibrate;
					uint32_t total_overshoot;
					uint32_t overshoot_samples;
					uint32_t calibrated_ticks;
				};

				class CallbackTickSource : public TickSource
				{
				public:
					CallbackTickSource(uint32_t scale);
					~CallbackTickSource() override;

					void DoTick();

				private:
					uint32_t _scale, _curr_scale;
				};

				class SubscriberTickSource : public TickSource
				{
				public:
					SubscriberTickSource(util::PubSubContext &context, PubSubType::PubSubType event, uint32_t scale = 1);
					~SubscriberTickSource();

					void DoTick();

				private:
					const util::PubSubscription *_subscription;
					uint32_t _scale, _curr_scale;
				};

			}
		}
	}
}

#endif /* TICKSOURCE_H_ */
