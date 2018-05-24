/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Interpreter.h
 * Author: harry
 *
 * Created on 24 May 2018, 14:02
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "core/thread/ThreadInstance.h"
#include "core/execution/ExecutionResult.h"
#include "util/LogContext.h"

UseLogContext(LogInterpreter)

namespace archsim {
	namespace interpret {
		class Interpreter {
		public:
			virtual ~Interpreter();
			virtual core::execution::ExecutionResult StepBlock(archsim::core::thread::ThreadInstance *thread) = 0;
		};
	}
}

#endif /* INTERPRETER_H */

