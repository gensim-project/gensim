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
#include "core/execution/ExecutionState.h"
#include "core/thread/ThreadInstance.h"

namespace archsim {
	namespace core {
		namespace execution {

			/**
			 * This class provides 'top-level' management of the execution of guest
			 * threads.
			 */
			class ExecutionContextManager {
			public:
				using EngineContainer = std::vector<ExecutionEngine*>;

				ExecutionContextManager();

				void AddEngine(ExecutionEngine *engine);

				ExecutionState GetState() const { return state_; }

				void SetTraceSink(libtrace::TraceSink *sink) { trace_sink_ = sink; }
				libtrace::TraceSink *GetTraceSink() { return trace_sink_; }

				void Start();
				void Halt();
				void Join();

				EngineContainer::iterator begin() { return contexts_.begin(); }
				EngineContainer::iterator end() { return contexts_.end(); }
				
			private:
				EngineContainer contexts_;
				ExecutionState state_;

				libtrace::TraceSink *trace_sink_;
			};
		}
	}
}

#endif /* EXECUTIONMANAGER_H */

