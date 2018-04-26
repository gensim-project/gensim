/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ExecutionManager.h
 * Author: harry
 *
 * Created on 11 April 2018, 15:35
 */

#ifndef EXECUTIONMANAGER_H
#define EXECUTIONMANAGER_H

#include "core/execution/ExecutionEngine.h"
#include "core/thread/ThreadInstance.h"

namespace archsim {
	namespace core {
		namespace execution {

			enum class ExecutionState {
				Unknown,
				Ready,
				Running,
				Halted,
				Aborted
			};

			/**
			 * This class is responsible for managing the execution of guest threads
			 * by using their registered execution engines. A single execution context
			 * can handle a single guest system and execution engine configuration.
			 */
			class ExecutionContext {
			public:
				using ThreadContainer = std::vector<archsim::core::thread::ThreadInstance*>;

				ExecutionContext(ArchDescriptor &arch, ExecutionEngine *engine);

				void AddThread(archsim::core::thread::ThreadInstance *thread);

				core::execution::ExecutionResult StepThreadsBlock();

				void SetTraceSink(libtrace::TraceSink *source) { trace_sink_ = source; }
				libtrace::TraceSink *GetTraceSink() { return trace_sink_; }

				ThreadContainer::iterator begin() { return local_threads_.begin(); }
				ThreadContainer::iterator end() { return local_threads_.end(); }

			private:
				ThreadContainer local_threads_;

				ArchDescriptor &arch_;
				ExecutionEngine *engine_;
				libtrace::TraceSink *trace_sink_;
			};

			/**
			 * This class provides 'top-level' management of the execution of guest
			 * threads.
			 */
			class ExecutionContextManager {
			public:
				using ContextContainer = std::vector<ExecutionContext*>;

				ExecutionContextManager();

				void AddContext(ExecutionContext *ctx);

				ExecutionState GetState() const { return state_; }

				void SetTraceSink(libtrace::TraceSink *sink) { trace_sink_ = sink; }
				libtrace::TraceSink *GetTraceSink() { return trace_sink_; }

				ContextContainer::iterator begin() { return contexts_.begin(); }
				ContextContainer::iterator end() { return contexts_.end(); }

				void StartSync();

			private:
				ContextContainer contexts_;
				ExecutionState state_;

				libtrace::TraceSink *trace_sink_;
			};
		}
	}
}

#endif /* EXECUTIONMANAGER_H */

