#include "libtrace/RecordTypes.h"
#include "libtrace/RecordFile.h"

#include <cstdio>
#include <cstdlib>

using namespace libtrace;

TraceRecord TR(Record r) {
	return *(TraceRecord*)&r;
}

InstructionHeaderRecord IH(Record r) {
	return *(InstructionHeaderRecord*)&r;
}

int main(int argc, char **argv)
{
	FILE *f1 = fopen(argv[1], "r");
	FILE *f2 = fopen(argv[2], "r");
	
	if(!f1 || !f2) {
		return 1;
	}
	
	RecordFile rf1 (f1);
	RecordFile rf2 (f2);
	
	uint64_t start1 = 0;
	uint64_t start2 = 0;
	
	if(argc == 5) {
		start1 = atoi(argv[3]);
		start2 = atoi(argv[4]);
	}
	
	auto it1 = rf1.begin();
	auto it2 = rf2.begin();
	
	uint64_t ctr1 = start1, ctr2 = start2;
	
	// seek to starts
	while(start1) {
		if(TR(*it1).GetType() == InstructionHeader) start1--;
		it1++;
	}
	
	while(start2) {
		if(TR(*it2).GetType() == InstructionHeader) start2--;
		it2++;
	}
	
	uint64_t diffed_count = 0;
	
	// scan until PC divergence
	while(true) {
	
		// make sure we are at an instruction header
		while(TR(*it1).GetType() != InstructionHeader) it1++;
		while(TR(*it2).GetType() != InstructionHeader) it2++;

		ctr1++;
		ctr2++;
		diffed_count++;

		if((diffed_count % 10000000) == 0) printf("%lu...\n", diffed_count);

		if(IH(*it1).GetPC() != IH(*it2).GetPC()) {
			printf("Divergence detected at instruction %lu %lu\n", ctr1, ctr2);
			return 1;
		}
		
		it1++;
		it2++;
	}
	
}
