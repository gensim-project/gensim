/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

namespace archsim
{
	namespace util
	{

		/*
		 * This event system is designed to solve several of the problems of
		 * the previous PubSub system, especially in a multithreaded context.
		 *
		 * The previous system has two main problems:
		 *  - Events might be received and processed by a subscriber while it
		 *    is in an indeterminate state (e.g., flushing decode cache while
		 *    using an instruction in said cache)
		 *  - Events cannot be scoped and so an event intended to affect one
		 *    thread actually affect all threads.
		 *
		 * This system is intended to solve these problems by introducing a
		 * concept of the event distributor:
		 *  1. An event distributor D is created
		 *  2. An event consumer C is created, and registered with D
		 *  3. An event producer P is created
		 *  4. P produces an event E, which C is interested in
		 *  5. The event E is placed into a buffer in D
		 *  6. D waits until it is in a safe state
		 *  7. D distributes E to C
		 *  8. C receives the event and acts on it
		 *
		 * Further, the producer of an event could require receipts, to ensure
		 * that all possible consumers of events receive and act before
		 * the producer returns/continues processing
		 *
		 *
		 *
		 */

		class EventDistributor
		{
		public:
			static EventDistributor &GetRootEventDistributor()
			{
				return root_event_distributor_;
			}
			using EventBufferedCallback = std::function<void (EventType, void *data) >;

			/*
			 * Register a consumer of events with a particular event type
			 */
			void RegisterConsumer(EventConsumer &consumer, EventType event_type);

			/*
			 * Post an event into the event system. Depending on the contents
			 * of the context, it might be propagated to other distributors
			 */
			bool PostEvent(EventContext &ctx);

			/*
			 * Process all events buffered at this distributor. This function
			 * assumes that the callee is 'safe' with regards to events, i.e.,
			 * no event consumer will cause the callee to go into an undefined
			 * state.
			 */
			void ProcessQueue();

			/*
			 * Set a callback to be invoked whenever a message is buffered.
			 * This callback will be invoked without this object's lock begin
			 * held.
			 */
			void SetEventBufferedCallback(const EventBufferedCallback &callback);

		private:
			void BufferEvent(const EventContext &event);

			std::mutex self_lock_;
			std::vector<EventContext> buffered_events_;
			std::map<EventType, std::vector<EventConsumer*>> event_consumers_;

			static EventDistributor root_event_distributor_;
		};

		class EventProducer
		{
		public:

		private:

		};

		class EventConsumer
		{
		public:
			using ConsumeCallback = std::function<void (EventType, void *data, void *context)>;

			void ConsumeEvent(const EventContext &event, void *data);
		private:

		};

		class EventContext
		{
		public:

		private:
			EventType event_type_;
			bool producer_awaiting_;
		};

	}
}