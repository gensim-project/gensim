/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "arch/ArchDescriptionParser.h"
#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/testing/SSAActionGenerator.h"
#include "genC/InstStructBuilder.h"

#include <iostream>
#include <fstream>
#include <random>

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
		fprintf(stderr, "Failed to parse asm\n");
		return 1;
	}
	

	gensim::genc::InstStructBuilder isb;
	ctx->GetTypeManager().InsertStructType("Instruction", isb.BuildType(isa));

	gensim::genc::ssa::io::ContextAssembler assembler;
	assembler.SetTarget(ctx);
	if(!assembler.Assemble(*afc, root_context)) {
		std::cerr << "Assembly failed" << std::endl;
		std::cerr << root_context;
		return 1;
	}

	gensim::genc::ssa::SSAPassManager manager;
	manager.SetMultirunAll(false);
	manager.SetMultirunEach(false);
	for(unsigned i = pass1_idx; i < argc; ++i) {
		std::string passname = argv[i];
		if(passname == "MultirunEach") {
			manager.SetMultirunEach(true);
		} else if(passname == "MultirunAll") {
			manager.SetMultirunAll(true);
		} else {
			gensim::genc::ssa::SSAPass *out;
			if(!GetComponentInstance(passname, out)) {
				fprintf(stderr, "Unknown pass: %s\n", passname.c_str());
				fputs(GetRegisteredComponents(out).c_str(), stderr);
				return 1;
			}
			manager.AddPass(out);
		}
	}
	manager.Run(*ctx);
	ctx->Resolve(root_context);
	std::cerr << root_context;
	
	gensim::genc::ssa::io::ContextDisassembler cd;
	std::ofstream outfile(output_file);
	cd.Disassemble(ctx, outfile);

	return 0;
}
