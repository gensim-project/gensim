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
	FILE *f = fopen(argv[1], "r");
	uint32_t pc = strtol(argv[2], NULL, 16);
	
	RecordFile rf(f);
	auto it = rf.begin();
	auto end = rf.end();
	
	uint64_t index = 0;
	uint32_t prev_pc = 0;
	
	while(it != end) {
		if(TR(*it).GetType() == InstructionHeader) {
			index++;
			if(IH(*it).GetPC() == pc) {
				printf("%llu (%08x)\n", index, prev_pc);
			}
			prev_pc = IH(*it).GetPC();
		}
		it++;
	}
	
	return 0;
}
