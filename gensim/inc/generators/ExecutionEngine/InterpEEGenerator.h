/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EEGenerator.h
 * Author: harry
 *
 * Created on 11 April 2018, 13:42
 */

#ifndef INTERPEEGENERATOR_H
#define INTERPEEGENERATOR_H

#include "genC/ssa/SSAFormAction.h"
#include "generators/ExecutionEngine/EEGenerator.h"

namespace gensim {
	namespace generator {
		
		class InterpEEGenerator : public EEGenerator  {
		public:
			InterpEEGenerator(GenerationManager &manager) : EEGenerator(manager, "interpreter") {}
			
			virtual bool GenerateHeader(util::cppformatstream &str) const;
			virtual bool GenerateSource(util::cppformatstream &str) const;

			~InterpEEGenerator();
		private:
			bool GenerateBlockExecutor(util::cppformatstream &str) const;
			
			bool GenerateDecodeInstruction(util::cppformatstream &str) const;
			bool GenerateHelperFunctions(util::cppformatstream &str) const;
			bool GenerateHelperFunction(util::cppformatstream &str, const isa::ISADescription &isa, const gensim::genc::ssa::SSAFormAction*) const;
			bool GenerateStepInstruction(util::cppformatstream &str) const;
			bool GenerateStepInstructionISA(util::cppformatstream &str, isa::ISADescription &isa) const;
			bool GenerateStepInstructionInsn(util::cppformatstream &str, isa::InstructionDescription &insn) const;
			
			bool GenerateBehavioursDescriptors(util::cppformatstream &str) const;
		};
		
	}
}

#endif /* EEGENERATOR_H */

