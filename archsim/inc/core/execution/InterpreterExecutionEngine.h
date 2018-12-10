/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   InterpreterExecutionEngine.h
 * Author: harry
 *
 * Created on 24 May 2018, 14:04
 */

#ifndef INTERPRETEREXECUTIONENGINE_H
#define INTERPRETEREXECUTIONENGINE_H

#include "core/execution/ExecutionEngine.h"
#include "interpret/Interpreter.h"
#include "module/Module.h"

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			class InterpreterExecutionEngineThreadContext : public ExecutionEngineThreadContext
			{
			public:
				InterpreterExecutionEngineThreadContext(ExecutionEngine *engine, thread::ThreadInstance *thread);
				~InterpreterExecutionEngineThreadContext();

				gensim::DecodeContext *GetDC()
				{
					return decode_ctx_;
				}

			private:
				gensim::DecodeContext *decode_ctx_;
			};

			class InterpreterExecutionEngine : public ExecutionEngine
			{
			public:
				InterpreterExecutionEngine(archsim::interpret::Interpreter *interpreter);

				ExecutionResult Execute(ExecutionEngineThreadContext* thread) override;
				ExecutionEngineThreadContext* GetNewContext(thread::ThreadInstance* thread) override;

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);

			private:
				archsim::interpret::Interpreter *interpreter_;
			};
		}
	}
}

#endif /* INTERPRETEREXECUTIONENGINE_H */

