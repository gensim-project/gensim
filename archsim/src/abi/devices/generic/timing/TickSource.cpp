/*
 * TickSource.cpp
 *
 *  Created on: 19 Jan 2015
 *      Author: harry
 */

#include "abi/devices/generic/timing/TickSource.h"
#include "abi/devices/generic/timing/TickConsumer.h"

#include <unistd.h>
#include <chrono>

using namespace archsim::abi::devices::timing;

TickConsumer::~TickConsumer() {}

TickSource::TickSource() : _tick_count(0) {}

TickSource::~TickSource() {}

void TickSource::AddConsumer(TickConsumer &consumer)
{
	consumers.insert(&consumer);
}

void TickSource::RemoveConsumer(TickConsumer &consumer)
{
	consumers.erase(&consumer);
}

void TickSource::Tick(uint32_t tick_periods)
{
	_tick_count++;
	for(auto *consumer : consumers) {
		consumer->Tick(tick_periods);
	}
}

MicrosecondTickSource::MicrosecondTickSource(uint32_t tick) : LoopThread("Microsecond Source"), ticks(tick), recalibrate(0), total_overshoot(0), overshoot_samples(0), calibrated_ticks(0)
{
	start();
}

MicrosecondTickSource::~MicrosecondTickSource()
{
	stop();
}

void MicrosecondTickSource::tick()
{
	Tick(1);

	if(recalibrate-- == 0) {
		recalibrate = 1000;

		// Attempt a calibration delay and measure how long it lasts
		auto before = std::chrono::high_resolution_clock::now();
		usleep(ticks);
		auto after = std::chrono::high_resolution_clock::now();

		int64_t delta_ticks = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count();

		//Figure out the overshoot. If we have overshot by a large amount then schedule another calibration soon
		int64_t overshoot = delta_ticks - ticks;
		if(overshoot > ticks) {
			overshoot = ticks / 2;
			recalibrate = 50;
		}

		total_overshoot += overshoot;
		if(overshoot_samples++ >  1000) {
			overshoot_samples = 1;
			total_overshoot = overshoot;
		}

		calibrated_ticks = ticks - (total_overshoot / overshoot_samples);

	} else {
		usleep(calibrated_ticks);
	}
}

CallbackTickSource::CallbackTickSource(uint32_t scale) : _scale(scale), _curr_scale(0) {}

CallbackTickSource::~CallbackTickSource()
{

}

void CallbackTickSource::DoTick()
{
	if(_curr_scale-- == 0) {
		Tick(1);
		_curr_scale = _scale;
	}
}

void subscriber_callback(PubSubType::PubSubType type, void *context, const void *data)
{
	SubscriberTickSource *ticker = (SubscriberTickSource*)context;
	ticker->DoTick();
}

SubscriberTickSource::SubscriberTickSource(util::PubSubContext& context, PubSubType::PubSubType event, uint32_t scale) : _curr_scale(scale), _scale(scale)
{
	_subscription = context.Subscribe(event, subscriber_callback, this);
}

SubscriberTickSource::~SubscriberTickSource()
{
	delete _subscription;
}

void SubscriberTickSource::DoTick()
{
	Tick(1);
}
