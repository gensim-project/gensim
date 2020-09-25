/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/RecordFile.h"
#include "libtrace/InstructionPrinter.h"
#include "libtrace/disasm/CapstoneDisassembler.h"

#include <iostream>
#include <cstdio>

using namespace libtrace;
using namespace libtrace::disasm;

int main(int argc, char **argv)
{
	if(argc == 1) {
		fprintf(stderr, "Usage: %s [-d <arch>] [record file]\n", argv[0]);
		return 1;
	}

	std::string arch;
	std::string filename;

	if (argv[1][0] == '-') {
		if (argv[1][1] == 'd') {
			if (argc != 4) {
				fprintf(stderr, "Usage: %s [-d <arch>] [record file]\n", argv[0]);
				return 1;
			} else {
				arch = argv[2];
				filename = argv[3];
			}
		} else {
			fprintf(stderr, "Usage: %s [-d <arch>] [record file]\n", argv[0]);
			return 1;
		}
	} else {
		filename = argv[1];
	}

	FILE *rfile = fopen(filename.c_str(), "r");
	if(!rfile) {
		perror("Could not open file");
		return 1;
	}

	Disassembler *disassembler = nullptr;

#if HAVE_CAPSTONE == 1
	if (arch != "") {
		disassembler = new CapstoneDisassembler(arch);
	}
#endif

	RecordFile rf (rfile);
	RecordBufferStreamAdaptor rbsa (&rf);
	TracePacketStreamAdaptor tpsa(&rbsa);

	auto begin = rf.begin();
	auto end = rf.end();

	InstructionPrinter ip;
	ip.SetDisassembler(disassembler);

	while(begin != end) {
		std::string insn;
		if(ip.TryFormat(&tpsa, insn)) {
			std::cout << insn << std::endl;
		} else {
			std::cout << "(end of valid stream)";
			break;
		}
	}

	delete disassembler;
	return 0;
}
