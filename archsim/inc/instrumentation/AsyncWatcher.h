/*
 * AsyncWatcher.h
 *
 *  Created on: 25 Jun 2015
 *      Author: harry
 */

#ifndef INC_UTIL_ASYNCWATCHER_H_
#define INC_UTIL_ASYNCWATCHER_H_

#include "concurrent/Thread.h"
#include "concurrent/ThreadsafeQueue.h"

#include "util/PubSubSync.h"

namespace archsim
{

	namespace util
	{

		class AsyncWatcher : private concurrent::Thread
		{
		public:
			AsyncWatcher(util::PubSubContext &pubsub, uint32_t data_size);
			void Subscribe(PubSubType::PubSubType type);

			virtual void ProcessElement(PubSubType::PubSubType type, void *data) = 0;
			void Enqueue(PubSubType::PubSubType type, void *data);

			void Start();
			void Finish();
			void Terminate();

			static const uint32_t max_data_size = 8;
			typedef uint8_t data_buffer_t[max_data_size];
		private:
			void run() override;

			struct queue_entry {
				PubSubType::PubSubType type;
				data_buffer_t data;
			};

			typedef concurrent::ThreadsafeQueue<struct queue_entry, 1024*1024, false> queue_t;
			queue_t *queue;
			util::PubSubContext &pubsub;
			volatile bool terminate;
		};

	}

}

#endif /* INC_UTIL_ASYNCWATCHER_H_ */
