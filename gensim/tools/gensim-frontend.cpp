/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <fstream>

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "arch/ArchDescriptionParser.h"
#include "isa/ISADescriptionParser.h"
#include "DiagnosticContext.h"
#include "genC/ssa/io/Disassemblers.h"

#include "Util.h"

#include "genC/Parser.h"

using namespace gensim;
using namespace gensim::util;

static void DumpDiagnostics(const DiagnosticContext& context)
{
	std::cout << context;
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [system file]\n", argv[0]);
		return 1;
	}

	std::string arch_name = argv[1];

	DiagnosticSource root_source("GenSim");
	DiagnosticContext root_context(root_source);

	arch::ArchDescriptionParser parser(root_context);
	if (!parser.ParseFile(arch_name)) {
		parser.Get()->PrettyPrint(std::cout);
		DumpDiagnostics(root_context);

		return 1;
	}

	arch::ArchDescription *description = parser.Get();

	bool success = true;
	for(auto isa : description->ISAs) {
		success &= isa->BuildSSAContext(description, root_context);
	}
	if(!success) {
		std::cout << root_context;
		return 1;
	}

	for (std::list<isa::ISADescription *>::const_iterator II = description->ISAs.begin(), IE = description->ISAs.end(); II != IE; ++II) {
		const isa::ISADescription *isa = *II;
		if (!isa->valid) {
			printf("Errors in arch description for ISA %s.\n", isa->ISAName.c_str());
		}

		gensim::genc::ssa::io::ContextDisassembler cd;
		std::ofstream file(isa->ISAName + ".gcs");
		cd.Disassemble(&isa->GetSSAContext(), file);
	}

	delete description;

	return 0;
}
