/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/RecordFile.h"
#include "libtrace/InstructionPrinter.h"

#include <iostream>
#include <cstdio>

using namespace libtrace;

int main(int argc, char **argv)
{
	if(argc == 1) {
		fprintf(stderr, "Usage: %s [record file]\n", argv[0]);
		return 1;
	}

	FILE *rfile = fopen(argv[1], "r");
	if(!rfile) {
		perror("Could not open file");
		return 1;
	}

	RecordFile rf (rfile);
	RecordBufferStreamAdaptor rbsa (&rf);
	TracePacketStreamAdaptor tpsa(&rbsa);

	auto begin = rf.begin();
	auto end = rf.end();

	InstructionPrinter ip;

	while(begin != end) {
		std::string insn;
		if(ip.TryFormat(&tpsa, insn)) {
			std::cout << insn << std::endl;
		} else {
			std::cout << "(end of valid stream)";
			break;
		}
	}

	return 0;
}
