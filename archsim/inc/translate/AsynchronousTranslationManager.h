/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   AsynchronousTranslationManager.h
 * Author: s0457958
 *
 * Created on 06 August 2014, 09:16
 */

#ifndef ASYNCHRONOUSTRANSLATIONMANAGER_H
#define	ASYNCHRONOUSTRANSLATIONMANAGER_H

#include "translate/TranslationManager.h"
#include "blockjit/BlockJitTranslate.h"
#include "gensim/gensim_translate.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <unordered_set>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace translate
	{
		namespace profile
		{
			class Region;
		}

		class AsynchronousTranslationWorker;

		class WorkUnitQueueComparator
		{
		public:
			bool operator()(const TranslationWorkUnit* lhs, const TranslationWorkUnit* rhs) const;
		};

		class AsynchronousTranslationManager : public TranslationManager
		{
			friend class AsynchronousTranslationWorker;

		public:
			AsynchronousTranslationManager(util::PubSubContext *psctx);
			~AsynchronousTranslationManager();

			bool Initialise(gensim::BaseLLVMTranslate *translate);
			void Destroy() override;

			bool UpdateThreshold() override;

			bool TranslateRegion(archsim::core::thread::ThreadInstance *cpu, profile::Region& rgn, uint32_t weight) override;

			void PrintStatistics(std::ostream& stream);

		private:
			/**
			 * List maintaining asynchronous worker threads.
			 */
			std::list<AsynchronousTranslationWorker *> workers;

			/**
			 * Lock and condition protecting the work unit queue.
			 */
			std::mutex work_unit_queue_lock;
			std::condition_variable work_unit_queue_cond;
			llvm::LLVMContext ctx_;

			std::vector<TranslationWorkUnit *> work_unit_queue_;
		};
	}
}

#endif	/* ASYNCHRONOUSTRANSLATIONMANAGER_H */

