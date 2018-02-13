#include "libtrace/RecordFile.h"
#include "libtrace/InstructionPrinter.h"

#include <iostream>
#include <cstdio>

using namespace libtrace;

TraceRecord TR(Record r) {
	return *(TraceRecord*)&r;
}

InstructionHeaderRecord IH(Record r) {
	return *(InstructionHeaderRecord*)&r;
}

RegWriteRecord RW(Record r) {
	return *(RegWriteRecord*)&r;
}

int main(int argc, char **argv)
{
	if(argc != 3) {
		fprintf(stderr, "Usage: %s [record file] [ASID]\n", argv[0]);
		return 1;
	}
	
	FILE *rfile = fopen(argv[1], "r");
	if(!rfile) {
		perror("Could not open file");
		return 1;
	}
	
	uint32_t desired_asid = strtol(argv[2], NULL, 0);
	uint32_t current_asid = 0;
	
	RecordFile rf (rfile);
	
	auto begin = rf.begin();
	auto end = rf.end();
	
	while(begin != end) {
		auto inst_it = begin;
		
		assert(TR(*begin).GetType() == InstructionHeader);
		
		do {
			Record r = *begin;
			if(current_asid == desired_asid)fwrite(&r, sizeof(Record), 1, stdout);
			begin++;
		} while(TR(*begin).GetType() != InstructionHeader && begin != end);
		
		do {
			if(TR(*inst_it).GetType() == RegWrite) {
				if(RW(*inst_it).GetRegNum() == 0xf0) {
					current_asid = RW(*inst_it).GetData();
				}
			}
			inst_it++;
		} while(TR(*inst_it).GetType() != InstructionHeader && inst_it != end);
	}
	
	return 0;
}
