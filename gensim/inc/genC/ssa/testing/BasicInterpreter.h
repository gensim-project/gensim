/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   BasicInterpreter.h
 * Author: harry
 *
 * Created on 19 September 2017, 08:42
 */

#ifndef BASICINTERPRETER_H
#define BASICINTERPRETER_H

#include "genC/ir/IRConstant.h"
#include "genC/ssa/testing/MachineState.h"
#include "genC/ssa/testing/SSAInterpreter.h"
#include "genC/ssa/SSAType.h"

namespace gensim
{
	namespace arch
	{
		class ArchDescription;
	}
	namespace genc
	{
		namespace ssa
		{
			class SSAFormAction;

			namespace testing
			{
				/**
				 * This is a wrapper around the Interpreter system which makes
				 * it easier to construct and interface with.
				 */
				class BasicInterpreter
				{
				public:
					BasicInterpreter(const arch::ArchDescription &arch);

					IRConstant GetRegisterState(uint32_t bank, uint32_t index);
					void SetRegisterState(uint32_t bank, uint32_t index, IRConstant value);

					void RandomiseInstruction(const gensim::genc::ssa::SSAType &inst_type);

					bool ExecuteAction(const SSAFormAction *action, const std::vector<IRConstant> &param_values = {});
				private:
					SSAInterpreter innerinterpreter_;
					const arch::ArchDescription &arch_;

					MachineState<BasicRegisterFileState,MemoryState> msi_;
				};
			}
		}
	}
}

#endif /* BASICINTERPRETER_H */

