/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/ssa/testing/SSAInterpreter.h"
#include "genC/ssa/validation/SSAActionValidator.h"
#include "arch/ArchDescription.h"
#include "arch/ArchDescriptionParser.h"
#include "arch/testing/TestArch.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/printers/SSAActionPrinter.h"
#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/InstStructBuilder.h"
#include "genC/ir/IRConstant.h"

#include "isa/ISADescription.h"

#include <iostream>
#include <cstdlib>
#include <typeinfo>
#include <random>

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

typedef  gensim::genc::ssa::testing::MachineState<gensim::genc::ssa::testing::BasicRegisterFileState, gensim::genc::ssa::testing::MemoryState> machine_state_t;

machine_state_t InterpretAction(gensim::genc::ssa::SSAFormAction *action, std::mt19937_64 r)
{
	machine_state_t machine_state;
	size_t regstate_size = action->Arch->GetRegFile().GetSize();
	machine_state.RegisterFile().SetSize(regstate_size);
	machine_state.RegisterFile().SetWrap(true);

	// randomize register file
	for(int i = 0; i < regstate_size; ++i) {
		machine_state.RegisterFile().Write8(i, r() & 0xff);
	}

	gensim::genc::ssa::testing::SSAInterpreter interp (action->Arch, machine_state);
	interp.SetTracing(false);

	std::vector<IRConstant> values;

	for(unsigned int i = 0; i < action->ParamSymbols.size(); ++i) {
		values.push_back(IRConstant::Integer(0));
	}

	auto &inst_type = action->GetContext().GetTypeManager().GetStructType("Instruction");

	auto fields = action->Isa->Get_Decode_Fields();
	for(auto member : inst_type.BaseType.StructType->Members) {
		uint64_t value = r() & ((1 << fields[member.Name]) - 1);
		interp.State().Instruction().SetField(member.Name, IRConstant::Integer(value));
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

void TestAction(const std::string &passname, gensim::genc::ssa::SSAFormAction *pre_opt, gensim::genc::ssa::SSAFormAction *post_opt, std::mt19937_64 &r)
{
	auto prestate = InterpretAction(pre_opt, r);
	auto poststate = InterpretAction(post_opt, r);

	if(!prestate.Equals(poststate)) {
		gensim::genc::ssa::printers::SSAActionPrinter post_opt_printer(*pre_opt), pre_opt_printer(*post_opt);

		std::ostringstream post_opt_text, pre_opt_text;
		post_opt_printer.Print(post_opt_text);
		pre_opt_printer.Print(pre_opt_text);

		printf("Error detected! Optimisation %s. Pre text: \n%s\n\n Post text:\n%s\n\n", passname.c_str(), pre_opt_text.str().c_str(), post_opt_text.str().c_str());

		printf("Prestate: \n%s\n", prestate.Dump().c_str());
		printf("Poststate: \n%s\n", poststate.Dump().c_str());
	}
}

void OptimiseAction(gensim::genc::ssa::SSAFormAction *action, int tries, std::mt19937_64 &r)
{
	using gensim::genc::ssa::SSAPass;
	using gensim::genc::ssa::SSAPassDB;

	static std::vector<const SSAPass *> passes = {
		SSAPassDB::Get("O4")
//		SSAPassDB::Get("ParameterRename")

	};

	gensim::genc::ssa::validation::SSAActionValidator validator;

	gensim::DiagnosticSource source("OptimiseAction");
	gensim::DiagnosticContext diag(source);

	bool changed = false;

	changed = false;

	gensim::genc::ssa::SSAFormAction *test_action = action->Clone();

	for(auto i : passes) {
		if(i == nullptr) {
			continue;
		}

		if(!validator.Run(test_action, diag)) {
			printf("Failed to validate\n");
			std::cout << diag;
			throw std::logic_error("");
		}
		if(!test_action->Resolve(diag)) {
			printf("Failed to resolve\n");
			std::cout << diag;
			throw std::logic_error("");
		}

		gensim::genc::ssa::SSAFormAction *pre_opt_action = test_action->Clone();

		changed |= i->Run(*test_action);

		if(!validator.Run(test_action, diag)) {
			printf("Failed to validate\n");
			std::cout << diag;

			std::cout << "Pass " << typeid(*i).name() << std::endl;

			gensim::genc::ssa::io::ActionDisassembler ad;
			std::cout << "Pre opt:\n";
			ad.Disassemble(pre_opt_action, std::cout);
			std::cout << "\n\nPost Opt:\n";
			ad.Disassemble(test_action, std::cout);

			throw std::logic_error("");
		}
		if(!test_action->Resolve(diag)) {
			printf("Failed to resolve\n");
			std::cout << diag;
			throw std::logic_error("");
		}

		for(int t = 0; t < tries; ++t) {
			r.seed(t);
			TestAction(typeid(*i).name(), pre_opt_action, test_action, r);
		}

		pre_opt_action->Destroy();
		delete pre_opt_action;
	}

	test_action->Destroy();
	delete test_action;
}

gensim::arch::ArchDescription *ParseArch(gensim::DiagnosticContext &diag, const std::string &filename)
{
	gensim::arch::ArchDescriptionParser parser(diag);
	if (!parser.ParseFile(filename)) {
		return nullptr;
	}

	gensim::arch::ArchDescription *description = parser.Get();
	return description;
}

/*
 *
 */
int main(int argc, char** argv)
{

	if(argc < 3) {
		fprintf(stderr, "Usage: %s [arch_description] [isa name] [input file] [pass1] [pass2]...\n", argv[0]);
		return 1;
	}

	const char *arch_description = argv[1];
	const char *isa_name = argv[2];
	const char *input_file = argv[3];
	unsigned pass1_idx = 4;

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto arch = ParseArch(root_context, arch_description);
	if(arch == nullptr) {
		std::cerr << "Failed to parse Arch" << std::endl << root_context;
		return 1;
	}

	gensim::isa::ISADescription *isa = arch->GetIsaByName(isa_name);
	if(isa == nullptr) {
		fprintf(stderr, "Could not find isa %s\n", isa_name);
		return 1;
	}

	gensim::genc::ssa::SSAContext *ctx = new gensim::genc::ssa::SSAContext(*isa, *arch);

	gensim::genc::ssa::io::AssemblyReader ar;
	gensim::genc::ssa::io::AssemblyFileContext *afc = nullptr;
	if(!ar.Parse(input_file, root_context, afc)) {
		std::cerr << root_context;
		fprintf(stderr, "Failed to parse assembly file\n");
		return 1;
	}

	gensim::genc::InstStructBuilder isb;
	ctx->GetTypeManager().InsertStructType("Instruction", isb.BuildType(isa));

	gensim::genc::ssa::io::ContextAssembler assembler;
	assembler.SetTarget(ctx);

	if(!assembler.Assemble(*afc)) {
		fprintf(stderr, "Failed to parse assembly tree\n");
		std::cerr << root_context;
		return 1;
	}

	if(!ctx->Validate(root_context)) {
		fprintf(stderr, "SSA failed validation\n");
		std::cerr << root_context;
		return 1;
	}

	if(!ctx->Resolve(root_context)) {
		fprintf(stderr, "Failed to resolve SSA\n");
		return 1;
	}

	std::mt19937_64 r;
	for(auto action : ctx->Actions()) {
		std::cout << "Testing " << action.second->GetName() << std::endl;
		OptimiseAction((gensim::genc::ssa::SSAFormAction*)action.second, 10, r);
	}

	return 0;
}
