/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/ssa/testing/SSAInterpreter.h"
#include "arch/ArchDescription.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/printers/SSAActionPrinter.h"
#include "genC/ir/IRConstant.h"
#include "genC/ssa/io/Disassemblers.h"

#include "isa/testing/TestISA.h"
#include "arch/testing/TestArch.h"

#include <iostream>
#include <cstdlib>
#include <typeinfo>
#include <random>

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

int main(int argc, char **argv)
{
	random_t random;

	if(argc == 1) {
		return 1;
	}

	// build a fake arch
	gensim::arch::ArchDescription *arch = gensim::arch::testing::GetTestArch();
	gensim::isa::ISADescription *isa = gensim::isa::testing::GetTestISA(false);

	random_t::result_type seed = strtoll(argv[1], nullptr, 10);
	random.seed(seed);

	gensim::DiagnosticSource source("OptTest");
	gensim::DiagnosticContext diag(source);

	gensim::genc::ssa::SSAContext ctx(*isa, *arch);

	gensim::genc::ssa::testing::SSAActionGenerator action_gen(random, 5, 5, false);
	auto action = action_gen.Generate(ctx, "gend_action");

	gensim::genc::ssa::io::ContextDisassembler cd;
	cd.Disassemble(&ctx, std::cout);
	return 0;
}