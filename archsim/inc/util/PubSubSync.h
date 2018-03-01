/*
 * PubSubSync.h
 *
 *  Created on: 28 Nov 2014
 *      Author: harry
 */

#ifndef PUBSUBSYNC_H_
#define PUBSUBSYNC_H_

#include <vector>
#include <map>
#include <iostream>

#include "abi/devices/Component.h"

#define DeclarePubType(x) x,
namespace PubSubType
{
	enum PubSubType {
#include "util/PubSubType.h"
	};
}
#undef DeclarePubType

namespace PubSubType
{
	std::string GetPubTypeName(PubSubType type);
}

using namespace archsim::abi::devices;

namespace archsim
{

	namespace util
	{

		class PubSubContext;

		typedef void(*PubSubCallback)(PubSubType::PubSubType, void *context, const void *data);

		class PubSubscription
		{
		public:
			PubSubscription(PubSubType::PubSubType type, PubSubCallback callback, void *context);
			inline void Notify(const void *data)
			{
				callback(type, context, data);
			}
		private:
			void *context;
			PubSubCallback callback;
			PubSubType::PubSubType type;

		public:
			inline PubSubType::PubSubType GetType() const
			{
				return type;
			}

			inline const void *GetContext()
			{
				return context;
			}
			inline PubSubCallback GetCallback()
			{
				return callback;
			}
		};

		class PubSubscriber
		{
		public:
			PubSubscriber(PubSubContext *ctx);
			~PubSubscriber();

			void Subscribe(PubSubType::PubSubType type, PubSubCallback callback, void *context);
			void Publish(PubSubType::PubSubType type, const void *data);
			void Unsubscribe(PubSubType::PubSubType type);
		private:
			std::vector<PubSubscription*> subscriptions;
			PubSubContext* pubsubcontext;
		};

		class PubSubContext;

		class PubSubInstance
		{
		public:
			friend class PubSubContext;

			PubSubInstance(PubSubType::PubSubType type);

			const PubSubscription* Subscribe(PubSubCallback callback, void *context);
			void Publish(const void *data);

			void Unsubscribe(const PubSubscription*);

			bool HasSubscribers() const
			{
				return subscriptions.size();
			}
		private:
			std::vector<PubSubscription*> subscriptions;
			PubSubType::PubSubType type;
			uint64_t publish_count;
			bool publishing;

		public:
			uint64_t GetPublishCount() const
			{
				return publish_count;
			}
		};

		class PubSubContext : public Component
		{
		public:
			PubSubContext();
			bool Initialise()
			{
				return true;
			}
			const PubSubscription *Subscribe(PubSubType::PubSubType type, PubSubCallback callback, void *context);
			void Publish(PubSubType::PubSubType type, const void *data);

			inline bool HasSubscribers(PubSubType::PubSubType type) const
			{
				if(_instances.at(type) == NULL) return false;
				return _instances.at(type)->HasSubscribers();
			}

			const std::vector<PubSubscription*> &GetSubscribers(PubSubType::PubSubType type);

			void Unsubscribe(const PubSubscription *);

			void PrintStatistics(std::ostream& stream);
		private:
			std::vector<PubSubInstance*> _instances;
		};

	}

}


#endif /* PUBSUBSYNC_H_ */
