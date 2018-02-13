/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ParallelWorker.h
 * Author: harry
 *
 * Created on 21 June 2017, 11:00
 */

#ifndef PARALLELWORKER_H
#define PARALLELWORKER_H

#include <thread>

#include "util/WorkQueue.h"

namespace gensim
{
	namespace util
	{

		template <typename T, typename P> class ParallelWorkThread : public std::thread
		{
		public:
			typedef WorkQueue<T, P> work_queue_t;

			ParallelWorkThread(work_queue_t &queue) : std::thread([&queue]()
			{
				while(!queue.empty()) {
					queue.work_and_dequeue();
				}
			}) {}


		};

		template<typename T, typename P> class ParallelWorker
		{
		public:
			typedef std::function<T(P)> work_fn_t;
			typedef WorkQueue<T, P> work_queue_t;

			ParallelWorker(work_queue_t &queue) : work_queue_(queue), thread_count_(1) {}

			void set_thread_count(int new_count)
			{
				thread_count_ = new_count;
			}

			void start()
			{
				while(threads_.size() < thread_count_) {
					threads_.push_back(new ParallelWorkThread<T,P>(work_queue_));
				}
			}

			void join()
			{
				while(!work_queue_.empty()) {
					pthread_yield();
				}
				for(auto &i : threads_) {
					i->join();
					delete i;
				}
				threads_.clear();
			}

		private:
			work_queue_t &work_queue_;
			std::vector<ParallelWorkThread<T,P>*> threads_;
			unsigned int thread_count_;

		};

	}
}

#endif /* PARALLELWORKER_H */

