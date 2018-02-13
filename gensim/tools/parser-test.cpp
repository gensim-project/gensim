/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "arch/ArchDescriptionParser.h"
#include "arch/RegisterFile.h"
#include "isa/ISADescription.h"
#include "isa/InstructionDescription.h"
#include "DiagnosticContext.h"
#include "genC/Parser.h"
#include "genC/ssa/SSAContext.h"
#include "UArchDescription.h"
#include "genC/ir/IRConstant.h"

#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/testing/MachineState.h"
#include "genC/ssa/testing/SSAInterpreter.h"

using namespace gensim;
using namespace gensim::genc;
using namespace gensim::util;

// Deterministally generate a simple architecture
arch::ArchDescription *GenerateArch()
{
	gensim::arch::ArchDescription *arch = new gensim::arch::ArchDescription();
	arch->wordsize = 32;

	arch::RegSpaceDescriptor *regspace = new arch::RegSpaceDescriptor(&arch->GetRegFile(), 64);
	auto pcslot = new arch::RegSlotViewDescriptor(regspace, "PC", "uint32", 4, 0);
	auto spslot = new arch::RegSlotViewDescriptor(regspace, "SP", "uint32", 4, 4);
	//auto rbbank = new arch::RegBankViewDescriptor(regspace, "RB", "uint32", 1, 16, 4, 4, 0);
	auto rbbank = new arch::RegBankViewDescriptor(regspace, "RB", 0, 16, 4, 1, 4, 4, "uint32");
	arch->GetRegFile().AddRegisterSpace(regspace);
	arch->GetRegFile().AddRegisterSlot(regspace, pcslot);
	arch->GetRegFile().AddRegisterSlot(regspace, spslot);
	arch->GetRegFile().AddRegisterBank(regspace, rbbank);

	return arch;
}

isa::ISADescription *GenerateISA()
{
	gensim::isa::ISADescription *isa = new gensim::isa::ISADescription(0);

	auto testformat = new isa::InstructionFormatDescription(*isa);

	testformat->SetName("testfmt");
	testformat->AddChunk(isa::InstructionFormatChunk("field1", 8, false, true));
	testformat->AddChunk(isa::InstructionFormatChunk("field2", 8, false, true));
	testformat->AddChunk(isa::InstructionFormatChunk("field3", 8, false, true));
	testformat->AddChunk(isa::InstructionFormatChunk("field4", 8, false, true));

	isa->AddFormat(testformat);

	auto testinsn = new isa::InstructionDescription("testinsn", *isa, testformat);
	testinsn->BehaviourName = "test_behaviour";
	isa->AddInstruction(testinsn);

	return isa;
}

typedef  gensim::genc::ssa::testing::MachineState<gensim::genc::ssa::testing::BasicRegisterFileState, gensim::genc::ssa::testing::MemoryState> machine_state_t;

static machine_state_t InterpretAction(gensim::genc::ssa::SSAFormAction *action)
{
	machine_state_t machine_state;
	machine_state.RegisterFile().SetSize(action->Arch->GetRegFile().GetSize());
	machine_state.RegisterFile().SetWrap(true);

	gensim::genc::ssa::testing::SSAInterpreter interp (action->Arch, machine_state);
	interp.SetTracing(false);

	machine_state.Instruction().SetField("field1", IRConstant::Integer(0xf0f0));
	machine_state.Instruction().SetField("field2", IRConstant::Integer(0xf0f0));
	machine_state.Instruction().SetField("field3", IRConstant::Integer(0xf0f0));
	machine_state.Instruction().SetField("field4", IRConstant::Integer(0xf0f0));

	std::vector<IRConstant> values;

	for(unsigned int i = 0; i < action->ParamSymbols.size(); ++i) {
		values.push_back(IRConstant::Integer(0));
	}

	auto result = interp.ExecuteAction(action, values);

	switch(result.Result) {
			using namespace gensim::genc::ssa::testing;
		case Interpret_Error:
			throw std::logic_error("Error in interpretation");
			break;
	}

	return machine_state;
}

static bool OptimiseAndTestAction(gensim::genc::ssa::SSAFormAction *action, gensim::genc::ssa::SSAPass *pass)
{
	auto pre_state = InterpretAction(action);

	pass->Run(*action);

	auto post_state = InterpretAction(action);

	if(!pre_state.Equals(post_state)) {
		fprintf(stderr, "ERROR DETECTED!\n");
		abort();
	}

	return false;
}

static void OptimiseAndTest(genc::ssa::SSAFormAction *action)
{
	using namespace gensim::genc::ssa;
	std::vector<SSAPass *> passes {
		GetComponent<SSAPass>("DeadCodeElimination"),
		GetComponent<SSAPass>("ControlFlowSimplification"),
		GetComponent<SSAPass>("BlockMerging"),
		GetComponent<SSAPass>("ConstantFolding"),
		GetComponent<SSAPass>("ConstantPropagation"),
		GetComponent<SSAPass>("ValuePropagation"),
		GetComponent<SSAPass>("LoadStoreElimination"),
		GetComponent<SSAPass>("Inlining")
	};

	bool changed = false;
	do {
		changed = false;
		for (auto i : passes) {
			bool passchanged;
			passchanged = OptimiseAndTestAction(action, i);;

			changed |= passchanged;
		}
	} while(changed);
}

int main(int argc, char **argv)
{
	DiagnosticSource root_source("GenSim");
	DiagnosticContext root_context(root_source);

	arch::ArchDescription *arch = GenerateArch();
	isa::ISADescription *isa = GenerateISA();

	genc::GenCContext genc_ctx (*arch, *isa, root_context);
	genc_ctx.AddFile(argv[1]);
	if(genc_ctx.Parse()) {
		genc::ssa::SSAContext *ssa_ctx = genc_ctx.EmitSSA();

		genc::ssa::SSAFormAction *action =	(genc::ssa::SSAFormAction *)ssa_ctx->GetAction("test_behaviour");
		bool success = action->Resolve(root_context);
		if(!success) {
			std::cout << root_context;
			return 1;
		}

		OptimiseAndTest(action);

		ssa_ctx->GetAction("test_behaviour")->Dump();
		return 0;
	} else {
		fprintf(stderr, "Parse failed\n");
		std::cout << root_context;
		return 1;
	}


}