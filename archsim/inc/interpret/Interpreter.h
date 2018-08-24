/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Interpreter.h
 * Author: harry
 *
 * Created on 24 May 2018, 14:02
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "core/execution/ExecutionEngine.h"
#include "core/execution/ExecutionResult.h"
#include "util/LogContext.h"

UseLogContext(LogInterpreter)

namespace archsim
{
	namespace interpret
	{
		class Interpreter
		{
		public:
			virtual ~Interpreter();
			virtual core::execution::ExecutionResult StepBlock(archsim::core::execution::ExecutionEngineThreadContext *thread) = 0;

		};
	}
}

#endif /* INTERPRETER_H */

