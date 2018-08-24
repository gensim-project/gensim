/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef DYNAMICTRANSLATIONGENERATOR_H
#define DYNAMICTRANSLATIONGENERATOR_H

#include "generators/GenerationManager.h"
#include "generators/TranslationGenerator.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "Util.h"

/*
 How this JIT module works:
 * This JIT module is based on a modified C called GenC. The modifications to this language make it
 * easier to implement instruction behaviours using it in a way which makes it easy to reason about
 * which parts of the instruction behaviour are runtime dependent and which are not.
 *
 * In order to interface with the rest of the simulator, we use a system similar to the
 * ClangLLVMTranslationGenerator to precompile memory and register access functions. These functions
 * are then placed in a 'precompiled module' which is linked against the code actually generated at
 * JIT time.

 */

namespace gensim
{
	namespace generator
	{

		class DynamicTranslationNodeWalkerFactory : public genc::ssa::SSAWalkerFactory
		{
			virtual genc::ssa::SSANodeWalker *Create(const genc::ssa::SSAStatement *statement);
		};

		class DynamicTranslationGenerator : public TranslationGenerator
		{
		public:
			static std::string GetLLVMValue(std::string typeName, std::string value);

			bool Generate() const;
			bool GenerateActionCodeGeneratorFunctions() const;
			bool GenerateTranslateInstructionFunction() const;
			bool GeneratePrecompiledModule() const;

			bool EmitFixedFunction(util::cppformatstream &cfile, genc::ssa::SSAFormAction &action, std::string name_prefix /* = "Execute_" */) const;
			bool EmitDynamicEmitter(util::cppformatstream &cfile, util::cppformatstream &hstream, const genc::ssa::SSAFormAction &action, std::string name_prefix /* = "Execute_" */) const;

			bool GenerateFixedBlockEmitter(util::cppformatstream &cfile, genc::ssa::SSABlock &block) const;
			bool GenerateDynamicBlockEmitter(util::cppformatstream &cfile, genc::ssa::SSABlock &block) const;

			bool GenerateVirtualRegisterFunctions(util::cppformatstream &cfile, util::cppformatstream &hfile) const;
			bool GenerateInstructionFunctionSelector(util::cppformatstream &cfile, util::cppformatstream &hfile) const;

			bool GenerateNonInlineFunctions(util::cppformatstream &cfile) const;
			bool GenerateNonInlineFunctionPrototypes(util::cppformatstream &cfile) const;

			bool GeneratePredicateEmitters(util::cppformatstream &cfile, util::cppformatstream &hfile) const;

			void emit_llvm_values_for_generic(util::cppformatstream &target, const std::map<std::string, std::string> fields) const;
			void emit_llvm_call(const isa::ISADescription &isa, util::cppformatstream &target, std::string function, std::string returnType, bool ret_call, bool quote_fn) const;
			void emit_llvm_predicate_caller(util::cppformatstream &) const;
			void emit_llvm_pre_caller(util::cppformatstream &) const;
			void emit_llvm_post_caller(util::cppformatstream &) const;

			void Reset();
			void Setup(GenerationSetupManager &Setup);
			std::string GetFunction() const;

			void GenerateGraphs() const;
			virtual ~DynamicTranslationGenerator();

			DynamicTranslationGenerator(GenerationManager &man) : TranslationGenerator(man, "translate_dynamic")
			{
				SetProperty("Clang", "clang");
			}

			const std::vector<std::string> GetSources() const;

		private:
			mutable std::vector<std::string> sources;

		};
	}
}

#endif /* DYNAMICTRANSLATIONGENERATOR_H */
