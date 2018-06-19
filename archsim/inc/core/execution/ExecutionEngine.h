/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/* 
 * File:   ExecutionEngine.h
 * Author: harry
 *
 * Created on 10 April 2018, 16:21
 */

#ifndef EXECUTIONENGINE_H
#define EXECUTIONENGINE_H

#include "util/LogContext.h"
#include "core/execution/ExecutionResult.h"
#include "core/execution/ExecutionState.h"
#include "translate/profile/RegionTable.h"
#include "translate/profile/CodeRegionTracker.h"
#include <libtrace/TraceSink.h>


UseLogContext(LogExecutionEngine);

#include <thread>
#include <map>
#include <mutex>
#include <set>

namespace gensim {
	class DecodeContext;
}

namespace archsim {
	namespace core {
		namespace thread {
			class ThreadInstance;
		}
		namespace execution {

			class ExecutionEngine;
			
			class ExecutionEngineThreadContext {
			public:
				ExecutionEngineThreadContext(ExecutionEngine *engine, thread::ThreadInstance *thread);
				~ExecutionEngineThreadContext();
				
				std::thread &GetWorker() { return worker_; }
				thread::ThreadInstance *GetThread() { return thread_; }
				ExecutionState GetState() { return state_; }
				
				void Start();
				void Halt();
				void Suspend();
				void Join();
				
			private:
				void Execute();
				
				std::mutex lock_;
				ExecutionState state_;
				std::thread worker_;
				ExecutionEngine *engine_;
				thread::ThreadInstance *thread_;
			};
			
			class ExecutionEngine {
			public:
				using ThreadContainer = std::set<thread::ThreadInstance*>;
				
				ExecutionEngine();
				
				void AttachThread(thread::ThreadInstance *thread);
				void DetachThread(thread::ThreadInstance *thread);
				
				ThreadContainer &GetThreads() { return threads_; }
				
				void Start();
				void Halt();
				void Suspend();
				void Join();
				
				void SetTraceSink(libtrace::TraceSink *sink) { trace_sink_ = sink; }
				libtrace::TraceSink *GetTraceSink() { return trace_sink_; }
				
			private:
				friend class ExecutionEngineThreadContext;
				virtual ExecutionResult Execute(ExecutionEngineThreadContext *thread) = 0;
				
				ExecutionEngineThreadContext *GetContext(thread::ThreadInstance *thread);
				virtual ExecutionEngineThreadContext *GetNewContext(thread::ThreadInstance *thread) = 0;
				
				libtrace::TraceSink *trace_sink_;
				
				ThreadContainer threads_;
				std::map<thread::ThreadInstance *, ExecutionEngineThreadContext *> thread_contexts_;
				std::mutex lock_;
				ExecutionState state_;
			};
		}
	}
}

#endif /* EXECUTIONENGINE_H */

