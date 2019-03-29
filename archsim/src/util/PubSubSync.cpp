/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PubSubSync.cpp
 *
 *  Created on: 28 Nov 2014
 *      Author: harry
 */

#include "util/PubSubSync.h"
#include "util/LogContext.h"

DeclareLogContext(LogPubSub, "PubSub");

#include <iostream>
#include <cassert>

using namespace archsim::util;

namespace PubSubType
{
	std::string GetPubTypeName(PubSubType type)
	{
#define DeclarePubType(x) case x: return #x;
		switch(type) {
#include "util/PubSubType.h"
		}
#undef DeclarePubType
		assert(false);
		return "???";
	}
}

PubSubscription::PubSubscription(PubSubType::PubSubType type, PubSubCallback callback, void *ctx) : type(type), callback(callback), context(ctx) {}

PubSubscriber::PubSubscriber(PubSubContext &context) : pubsubcontext(context) {}

PubSubscriber::~PubSubscriber()
{
	for(auto i : subscriptions) {
		pubsubcontext.Unsubscribe(i);
	}
}

void PubSubscriber::Subscribe(PubSubType::PubSubType type, PubSubCallback callback, void *context)
{
	auto subscription = pubsubcontext.Subscribe(type, callback, context);
	subscriptions.push_back(subscription);
}

void PubSubscriber::Publish(PubSubType::PubSubType type, const void *data)
{
	pubsubcontext.Publish(type, data);
}

void PubSubscriber::Unsubscribe(PubSubType::PubSubType type)
{
	for(auto i = subscriptions.begin(); i != subscriptions.end(); ++i) {
		auto sub = *i;
		if(sub->GetType() == type) {
			pubsubcontext.Unsubscribe(sub);
			subscriptions.erase(i);
			return;
		}
	}
}

PubSubInstance::PubSubInstance(PubSubType::PubSubType type) : type(type), publish_count(0), publishing(0) {}


PubSubscription *PubSubInstance::Subscribe(PubSubCallback callback, void *context)
{
	std::lock_guard<std::mutex> lock(lock_);

	PubSubscription *subscription = new PubSubscription(type, callback, context);
	subscriptions.push_back(subscription);
	return subscription;
}

void PubSubInstance::Publish(const void *data)
{
	std::lock_guard<std::mutex> lock(lock_);

	publish_count++;
	if(publishing) {
		LC_ERROR(LogPubSub) << "Recursive publication detected!";
		assert(false);
	}
	publishing = true;
	for(auto sub : subscriptions) sub->Notify(data);
	publishing = false;
}

void PubSubInstance::Unsubscribe(const PubSubscription *sub)
{
	std::lock_guard<std::mutex> lock(lock_);

	for(auto i = subscriptions.begin(); i != subscriptions.end(); ++i) {
		if(*i == sub)  {
			subscriptions.erase(i);
			delete sub;
			return;
		}
	}
}

static ComponentDescriptor pubsubcontext_descriptor ("PubSubContext");

PubSubContext::PubSubContext() : _instances(PubSubType::_END, NULL), Component(pubsubcontext_descriptor)
{

}

PubSubscription *PubSubContext::Subscribe(PubSubType::PubSubType type, PubSubCallback callback, void *context)
{
	PubSubInstance *& instance = _instances[type];
	if(!instance) instance = new PubSubInstance(type);
	return instance->Subscribe(callback, context);
}

void PubSubContext::Publish(PubSubType::PubSubType type, const void *data)
{
	LC_DEBUG4(LogPubSub) << "Publishing " << PubSubType::GetPubTypeName(type) << " " << std::hex << (uint64_t)data;

	PubSubInstance *instance = _instances[type];
	if(!instance) return;
	instance->Publish(data);
}

const std::vector<PubSubscription*> &PubSubContext::GetSubscribers(PubSubType::PubSubType type)
{
	PubSubInstance *& instance = _instances[type];
	if(!instance) instance = new PubSubInstance(type);
	return instance->subscriptions;
}

void PubSubContext::Unsubscribe(const PubSubscription *sub)
{
	PubSubInstance *instance = _instances[sub->GetType()];
	assert(instance);
	instance->Unsubscribe(sub);
}

void PubSubContext::PrintStatistics(std::ostream &stream)
{
//	for(auto event : _instances) {
//		if(event)
//			stream << event.first << "\t" << event.second->GetPublishCount() << std::endl;
//	}
}
