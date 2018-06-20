/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef INTERPEEGENERATOR_H
#define INTERPEEGENERATOR_H

#include "genC/ssa/SSAFormAction.h"
#include "generators/ExecutionEngine/EEGenerator.h"

namespace gensim
{
	namespace generator
	{

		class InterpEEGenerator : public EEGenerator
		{
		public:
			InterpEEGenerator(GenerationManager &manager) : EEGenerator(manager, "interpreter")
			{
				manager.AddModuleEntry(ModuleEntry("Interpreter", "gensim::" + manager.GetArch().Name + "::Interpreter", "ee_interpreter.h", ModuleEntryType::Interpreter));
			}

			virtual bool GenerateHeader(util::cppformatstream &str) const;
			virtual bool GenerateSource(util::cppformatstream &str) const;

			void Setup(GenerationSetupManager& Setup) override;

			~InterpEEGenerator();
		private:
			bool GenerateBlockExecutor(util::cppformatstream &str) const;

			bool GenerateDecodeInstruction(util::cppformatstream &str) const;
			bool GenerateHelperFunctions(util::cppformatstream &str) const;
			bool GenerateHelperFunction(util::cppformatstream &str, const isa::ISADescription &isa, const gensim::genc::ssa::SSAFormAction*) const;
			bool GenerateStepInstruction(util::cppformatstream &str) const;
			bool GenerateStepInstructionISA(util::cppformatstream &str, isa::ISADescription &isa) const;
			bool RegisterStepInstruction(isa::InstructionDescription &insn) const;

			bool GenerateBehavioursDescriptors(util::cppformatstream &str) const;
		};

	}
}

#endif /* EEGENERATOR_H */

