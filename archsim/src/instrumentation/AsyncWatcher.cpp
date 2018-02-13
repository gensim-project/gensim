/*
 * AsyncWatcher.cpp
 *
 *  Created on: 25 Jun 2015
 *      Author: harry
 */

#include "util/AsyncWatcher.h"

#include <unistd.h>

using namespace archsim::util;

static void callback(PubSubType::PubSubType type, const void *ctx, const void *data)
{
	AsyncWatcher *watcher = (AsyncWatcher*)ctx;

	AsyncWatcher::data_buffer_t buffer;
	memcpy(buffer, data, AsyncWatcher::max_data_size);

	watcher->Enqueue(type, data);
}

AsyncWatcher::AsyncWatcher(PubSubContext &pubsub, uint32_t data_size) : pubsub(pubsub), terminate(false)
{
	assert(data_size < max_data_size);
	queue = new queue_t();
}

void AsyncWatcher::Subscribe(PubSubType::PubSubType type)
{
	pubsub.Subscribe(type, callback, this);
}

inline void AsyncWatcher::Enqueue(PubSubType::PubSubType type, void *data)
{
	struct queue_entry entry;
	entry.type = type;
	entry.data = data;
	queue->push(entry);
}

void AsyncWatcher::Start()
{
	start();
}

void AsyncWatcher::Finish()
{
	while(!queue->empty()) usleep(1000);
	Terminate();
}

void AsyncWatcher::Terminate()
{
	terminate = true;
	join();
}

void AsyncWatcher::run()
{
	while(true) {
		while(!terminate && queue->empty()) asm("pause");
		if(terminate) return;

		auto element = queue->front();
		queue->pop();
		ProcessElement(element.type, element.data);
	}
}
