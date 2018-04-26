/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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


UseLogContext(LogExecutionEngine);

#include <thread>
#include <map>

namespace gensim {
	class DecodeContext;
}

namespace archsim {
	namespace core {
		namespace thread {
			class ThreadInstance;
		}
		namespace execution {

			class ExecutionEngine {
			public:
				virtual void StartAsyncThread(thread::ThreadInstance *thread) = 0;
				virtual void HaltAsyncThread(thread::ThreadInstance *thread) = 0;
				virtual core::execution::ExecutionResult JoinAsyncThread(thread::ThreadInstance *thread) = 0;

				virtual core::execution::ExecutionResult StepThreadSingle(thread::ThreadInstance *thread) = 0;
				virtual core::execution::ExecutionResult StepThreadBlock(thread::ThreadInstance *thread) = 0;
				virtual core::execution::ExecutionResult Execute(thread::ThreadInstance *thread) = 0;

				virtual void TakeException(thread::ThreadInstance *thread, uint64_t category, uint64_t data) = 0;
			};

			class BasicExecutionEngine : public ExecutionEngine {
			public:
				BasicExecutionEngine();

				virtual void StartAsyncThread(thread::ThreadInstance* thread);
				virtual void HaltAsyncThread(thread::ThreadInstance* thread);
				virtual core::execution::ExecutionResult JoinAsyncThread(thread::ThreadInstance* thread);

				virtual core::execution::ExecutionResult Execute(thread::ThreadInstance *thread);
				virtual core::execution::ExecutionResult StepThreadBlock(thread::ThreadInstance *thread);
				virtual core::execution::ExecutionResult StepThreadSingle(thread::ThreadInstance *thread);

				void TakeException(thread::ThreadInstance* thread, uint64_t category, uint64_t data) override;

				class ExecutionContext {
				public:
					std::thread *thread;
					volatile bool should_halt;
					core::execution::ExecutionResult last_result;
					thread::ThreadInstance *thread_instance;
					BasicExecutionEngine *engine;
				};
			protected:
				gensim::DecodeContext *decode_context_;	
			private:
				virtual core::execution::ExecutionResult ArchStepBlock(thread::ThreadInstance *thread) = 0;
				virtual core::execution::ExecutionResult ArchStepSingle(thread::ThreadInstance *thread) = 0;

				std::map<thread::ThreadInstance*, ExecutionContext*> threads_;


			};
		}
	}
}

#endif /* EXECUTIONENGINE_H */

