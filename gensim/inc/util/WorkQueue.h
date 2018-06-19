/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   WorkQueue.h
 * Author: harry
 *
 * Created on 21 June 2017, 10:16
 */

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <vector>
#include <functional>
#include <mutex>

namespace gensim
{
	namespace util
	{
		template<typename T, typename P> class WorkQueue
		{
		public:
			typedef std::function<T(P)> work_fn_t;
			typedef std::vector<P> work_list_t;

			WorkQueue(const work_fn_t work_fn) : work_fn_(work_fn) {}

			void enqueue(const P& p)
			{
				std::lock_guard<std::mutex> lock(mtx_);
				work_list_.push_back(p);
			}

			void work_and_dequeue()
			{
				mtx_.lock();
				if(empty()) {
					return;
				}
				P back = work_list_.back();
				work_list_.pop_back();
				mtx_.unlock();

				work_fn_(back);
			}

			size_t size() const
			{
				return work_list_.size();
			}

			bool empty() const
			{
				return size() == 0;
			}

		private:
			work_fn_t work_fn_;
			work_list_t work_list_;
			std::mutex mtx_;
		};
	}
}

#endif /* WORKQUEUE_H */

