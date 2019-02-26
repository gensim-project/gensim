/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _LLVM_COMPILER_H
#define _LLVM_COMPILER_H

#include "translate/llvm/LLVMTranslation.h"
#include "translate/llvm/LLVMMemoryManager.h"
#include "translate/TranslationWorkUnit.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMCompiledModuleHandle
			{
			public:
				LLVMCompiledModuleHandle(llvm::orc::JITDylib *lib, llvm::orc::VModuleKey key) : Lib(lib), Key(key) {}

				llvm::orc::JITDylib *Lib;
				llvm::orc::VModuleKey Key;
			};

			class LLVMCompiler
			{
			public:
				using LinkLayer = llvm::orc::RTDyldObjectLinkingLayer;
				using CompileLayer = llvm::orc::IRCompileLayer;

				LLVMCompiler(llvm::orc::ThreadSafeContext &ctx);

				LLVMCompiledModuleHandle AddModule(llvm::Module *module);

				LLVMTranslation *GetTranslation(LLVMCompiledModuleHandle &module, TranslationWorkUnit &twu);

				void GC()
				{
					code_pool.GC();
				}

				llvm::orc::ThreadSafeContext &GetContext()
				{
					return ctx_;
				}
			private:
				void initJitSymbols();

				using SymbolResolver = std::function<llvm::JITSymbol(std::string)>;

				std::unique_ptr<llvm::TargetMachine> target_machine_;
				llvm::orc::ExecutionSession session_;
				LinkLayer linker_;
				std::unique_ptr<CompileLayer> compiler_;
				llvm::orc::ThreadSafeContext &ctx_;
				std::map<std::string, void *> jit_symbols_;

				util::PagePool code_pool;
			};
		}
	}
}

#endif
