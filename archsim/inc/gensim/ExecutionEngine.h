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

#include "ThreadInstance.h"
#include "util/LogContext.h"

UseLogContext(LogExecutionEngine);

#include <thread>
#include <map>

namespace archsim {
		enum class ExecutionResult {
			Continue,
			Exception,
			Abort,
			Halt
		};
		
		class ExecutionEngine {
		public:
			virtual void StartAsyncThread(ThreadInstance *thread) = 0;
			virtual void HaltAsyncThread(ThreadInstance *thread) = 0;
			virtual ExecutionResult JoinAsyncThread(ThreadInstance *thread) = 0;
			
			virtual ExecutionResult StepThreadSingle(ThreadInstance *thread) = 0;
			virtual ExecutionResult StepThreadBlock(ThreadInstance *thread) = 0;
			virtual ExecutionResult Execute(ThreadInstance *thread) = 0;
			
			virtual void TakeException(ThreadInstance *thread, uint64_t category, uint64_t data) = 0;
		};
		
		class BasicExecutionEngine : public ExecutionEngine {
		public:
			virtual void StartAsyncThread(ThreadInstance* thread);
			virtual void HaltAsyncThread(ThreadInstance* thread);
			virtual ExecutionResult JoinAsyncThread(ThreadInstance* thread);
			
			virtual ExecutionResult Execute(ThreadInstance *thread);
			virtual ExecutionResult StepThreadBlock(ThreadInstance *thread);
			virtual ExecutionResult StepThreadSingle(ThreadInstance *thread);
			
			void TakeException(ThreadInstance* thread, uint64_t category, uint64_t data) override;

			class ExecutionContext {
			public:
				std::thread *thread;
				volatile bool should_halt;
				ExecutionResult last_result;
				ThreadInstance *thread_instance;
				BasicExecutionEngine *engine;
			};
		private:
			virtual ExecutionResult ArchStepBlock(ThreadInstance *thread) = 0;
			virtual ExecutionResult ArchStepSingle(ThreadInstance *thread) = 0;
			
			std::map<ThreadInstance*, ExecutionContext*> threads_;
		};
}

#endif /* EXECUTIONENGINE_H */

