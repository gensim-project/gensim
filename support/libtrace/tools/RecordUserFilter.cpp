#include "libtrace/RecordTypes.h"
#include "libtrace/RecordFile.h"
#include "libtrace/InstructionPrinter.h"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <vector>

using namespace libtrace;

int main(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "Usage: %s [input file]\n", argv[0]);
		return 1;
	}
	
	FILE *f = fopen(argv[1], "r");
	if(!f) {
		perror("Could not open input file");
		return 1;
	}
	RecordFile rf (f);
	
	auto it = rf.begin();
	auto end = rf.end();
	
	std::vector<Record> outdata;
	InstructionPrinter ip;
	
	while(it != end) {
		// if we have an instruction with a kernel-mode PC (0xcxxxxxxx), skip it
		Record record = *it;
		TraceRecord *tr = (TraceRecord*)&record;
		
		bool print = true;
		
		if(tr->GetType() == InstructionHeader) {
			InstructionHeaderRecord *ihr = (InstructionHeaderRecord*)tr;
			if(((ihr->GetPC() & 0xf0000000) == 0xc0000000) || ((ihr->GetPC() & 0xf0000000) == 0xf0000000)) {
				print = false;
			}
		}
		
//		auto c = it;
//		std::cerr << ip(it) << std::endl;
		
		do {
			if(print) outdata.push_back(*tr);
			
			it++;
			record = *it;
			tr = (TraceRecord*)&record;
			
		} while(tr->GetType() != InstructionHeader && it != end);
		
		if(print) {
			fwrite(outdata.data(), sizeof(Record), outdata.size(), stdout);
			outdata.clear();
		}
	}
	
	return 0;
}
