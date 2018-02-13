/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   opt-test.cpp
 * Author: harry
 *
 * Created on 22 April 2017, 15:37
 */

#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/ssa/testing/SSAInterpreter.h"
#include "arch/ArchDescription.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSACloner.h"
#include "genC/ssa/printers/SSAActionPrinter.h"

#include "isa/testing/TestISA.h"
#include "arch/testing/TestArch.h"

#include <cstdlib>
#include <typeinfo>

using gensim::genc::IRConstant;

class coolrandom : public std::mt19937_64
{
public:
	result_type operator()()
	{
		return std::mt19937_64::operator()();
	}

	result_type operator()(result_type max)
	{
		return operator()() % max;
	}

	result_type operator()(result_type min, result_type max)
	{
		return min + (operator()() % (max-min));
	}
};

typedef coolrandom random_t;

gensim::arch::RegSlotViewDescriptor *GenerateRegSlot(gensim::arch::RegSpaceDescriptor *parent, random_t &random)
{
	std::string id = "slot_" + std::to_string(random());

	int type_width = 1 << (random() % 4);
	std::string type = "uint" + std::to_string(8*type_width);

	int width = type_width;
	int offset = random() % parent->GetSize();
	if(offset > parent->GetSize()-width) {
		offset = parent->GetSize() - width;
	}

	return new gensim::arch::RegSlotViewDescriptor(parent, id, type, width, offset);
}

gensim::arch::RegBankViewDescriptor *GenerateRegBank(gensim::arch::RegSpaceDescriptor *parent, random_t &random)
{
	std::string id = "bank_" + std::to_string(random());

	int type_width_in_bytes = 1 << (random() % 4);
	std::string type = "uint" + std::to_string(8*type_width_in_bytes);

	int elem_count = 1;

	int elem_size = type_width_in_bytes;
	int elem_stride = type_width_in_bytes;
	int register_stride = elem_count * elem_stride;
	
	int register_count = random() % (parent->GetSize() / register_stride);
	
	int offset = 0;
	return new gensim::arch::RegBankViewDescriptor(parent, id, offset, register_count, register_stride, elem_count, elem_size, elem_stride, type);
}

gensim::arch::RegSpaceDescriptor *GenerateRegSpace(gensim::arch::RegisterFile *parent, random_t &random)
{
	gensim::arch::RegSpaceDescriptor *space = new gensim::arch::RegSpaceDescriptor(parent, random(30,100));

	// add a random number of register slots
	int register_slots = random(1,3);
	while(register_slots--) {
		if(!parent->AddRegisterSlot(space, GenerateRegSlot(space, random))) {
			throw std::logic_error("");
		}
	}

	// add a random number of register banks
	int register_banks = random(1,3);
	while(register_banks--) {
		if(!parent->AddRegisterBank(space, GenerateRegBank(space, random))) {
			throw std::logic_error("");
		}
	}

	return space;
}

gensim::arch::ArchDescription *GenerateArch(random_t &random)
{
	gensim::arch::ArchDescription *arch = new gensim::arch::ArchDescription();

	// add a random number of register spaces
	int reg_space_count = random(1,3);
	while(reg_space_count--) {
		arch->GetRegFile().AddRegisterSpace(GenerateRegSpace(&arch->GetRegFile(), random));
	}

	return arch;
}

typedef  gensim::genc::ssa::testing::MachineState<gensim::genc::ssa::testing::BasicRegisterFileState, gensim::genc::ssa::testing::MemoryState> machine_state_t;

machine_state_t InterpretAction(gensim::genc::ssa::SSAFormAction *action)
{
	machine_state_t machine_state;
	machine_state.RegisterFile().SetSize(action->Arch->GetRegFile().GetSize());
	machine_state.RegisterFile().SetWrap(true);
	gensim::genc::ssa::testing::SSAInterpreter interp (action->Arch, machine_state);
	interp.SetTracing(false);

	std::vector<IRConstant> param_values;
	for(unsigned i = 0; i < action->ParamSymbols.size(); ++i) {
		param_values.push_back(IRConstant::Integer(0));
	}
	auto result = interp.ExecuteAction(action, param_values);

	switch(result.Result) {
			using namespace gensim::genc::ssa::testing;
		case Interpret_Error:
			throw std::logic_error("Error in interpretation");
			break;
	}

	return machine_state;
}

void PrintDiag(const gensim::DiagnosticContext &diag)
{
	for(auto i : diag.GetEntries()) {
		printf("%s\n", i.GetMessage().c_str());
	}
}



/*
 *
 */
int main(int argc, char** argv)
{

	random_t random;

	random.seed(time(nullptr));

	// build a fake arch
	gensim::arch::ArchDescription *arch = gensim::arch::testing::GetTestArch();
	gensim::isa::ISADescription *isa = gensim::isa::testing::GetTestISA(false);
	gensim::DiagnosticSource diag_src("CloneTest");
	gensim::DiagnosticContext diagnostic(diag_src);
	gensim::genc::ssa::SSAContext ssa_context(*isa, *arch);

	int count = 10000;
	while(count--) {
		gensim::genc::ssa::testing::SSAActionGenerator action_gen(random, 5, 10, false);

		auto action = action_gen.Generate(ssa_context, "gend_action");

		gensim::genc::ssa::SSACloner cloner;
		auto cloned_action = cloner.Clone(action);

		auto orig_state = InterpretAction(action);
		std::ostringstream orig_dump;

		gensim::genc::ssa::printers::SSAActionPrinter orig_action_printer(*action);
		orig_action_printer.Print(orig_dump);

		action->Destroy();
		delete action;

		auto cloned_state = InterpretAction(cloned_action);
		std::ostringstream cloned_dump;

		gensim::genc::ssa::printers::SSAActionPrinter cloned_action_printer(*cloned_action);
		cloned_action_printer.Print(cloned_dump);

		if(!orig_state.Equals(cloned_state)) {
			fprintf(stderr, "Original: %s\n", orig_state.Dump().c_str());
			fprintf(stderr, "Cloned: %s\n", cloned_state.Dump().c_str());

			fprintf(stderr, "Original: \n%s\n\n", orig_dump.str().c_str());
			fprintf(stderr, "Cloned: \n%s\n\n", cloned_dump.str().c_str());

			fprintf(stderr, "Clone failed!\n");
			throw std::logic_error("");
		}

		cloned_action->Destroy();
		delete cloned_action;

	}

	fprintf(stderr, "OK\n");
	return 0;
}
