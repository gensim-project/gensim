/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "arch/ArchDescriptionParser.h"
#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/InstStructBuilder.h"

#include "ssa_asm/ssa_asmLexer.h"
#include "ssa_asm/ssa_asmParser.h"

#include <iostream>
#include <fstream>
#include <random>

static pANTLR3_INPUT_STREAM GetFile(const std::string &str)
{
	FILE *f = fopen(str.c_str(), "r");
	if(f == nullptr) {
		return nullptr;
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *data = (char*)malloc(size);
	fread(data, 1, size, f);

	// check data
	for(size_t i = 0; i < size; ++i) {
		char c= data[i];
		if(!isascii(c)) {
			return nullptr;
		}
	}

	return antlr3StringStreamNew((pANTLR3_UINT8)data, ANTLR3_ENC_8BIT, size, (pANTLR3_UINT8)strdup(str.c_str()));
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

int main(int argc, char **argv)
{
	if(argc < 4) {
		fprintf(stderr, "Usage: %s [arch_description] [isa name] [input file] [output_file] [pass1] [pass2]...\n", argv[0]);
		return 1;
	}

	const char *arch_description = argv[1];
	const char *isa_name = argv[2];
	const char *input_file = argv[3];
	const char *output_file = argv[4];
	unsigned pass1_idx = 5;

	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto arch = ParseArch(root_context, arch_description);

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

	for(auto i : ctx->Actions()) {
		((gensim::genc::ssa::SSAFormAction*)i.second)->Dump(std::cout);
	}

	return 0;
}
