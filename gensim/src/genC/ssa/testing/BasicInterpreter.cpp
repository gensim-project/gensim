/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "genC/ssa/testing/BasicInterpreter.h"
#include "genC/ssa/SSAType.h"
#include "define.h"

#include <random>

using namespace gensim::genc::ssa::testing;

BasicInterpreter::BasicInterpreter(const gensim::arch::ArchDescription& arch) : arch_(arch), innerinterpreter_(&arch, msi_)
{
	msi_.RegisterFile().SetSize(arch_.GetRegFile().GetSize());
	msi_.RegisterFile().SetWrap(false);
}

gensim::genc::IRConstant BasicInterpreter::GetRegisterState(uint32_t bank, uint32_t index)
{
	// look up register bank descriptor
	const auto &descriptor = arch_.GetRegFile().GetBankByIdx(bank);
	uint32_t base_offset = descriptor.GetRegFileOffset();
	uint32_t reg_offset = descriptor.GetElementSize() * index;

	return IRConstant::Integer(msi_.RegisterFile().Read32(base_offset + reg_offset));
}

void BasicInterpreter::SetRegisterState(uint32_t bank, uint32_t index, gensim::genc::IRConstant value)
{
	// look up register bank descriptor
	const auto &descriptor = arch_.GetRegFile().GetBankByIdx(bank);
	uint32_t base_offset = descriptor.GetRegFileOffset();
	uint32_t reg_offset = descriptor.GetElementSize() * index;

	return msi_.RegisterFile().Write32(base_offset + reg_offset, value.Int());
}

void BasicInterpreter::RandomiseInstruction(const gensim::genc::ssa::SSAType &inst_type)
{
	std::mt19937_64 random;

	GASSERT(inst_type.IsStruct());
	auto structtype = inst_type.BaseType.StructType;

	for(auto member : structtype->Members) {
		innerinterpreter_.State().Instruction().SetField(member.Name, IRConstant::Integer(random()));
	}
}


bool BasicInterpreter::ExecuteAction(const SSAFormAction* action, const std::vector<IRConstant> &param_values)
{
	auto result = innerinterpreter_.ExecuteAction(action, param_values);
	return result.Result ==	Interpret_Normal;
}
