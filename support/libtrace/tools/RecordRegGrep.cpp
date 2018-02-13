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

RegWriteRecord RW(Record r) {
	return *(RegWriteRecord*)&r;
}
RegReadRecord RR(Record r) {
	return *(RegReadRecord*)&r;
}


int main(int argc, char **argv)
{
	FILE *f = fopen(argv[1], "r");
	uint32_t pc = strtol(argv[2], NULL, 10);
	
	RecordFile rf(f);
	auto it = rf.begin();
	auto end = rf.end();
	
	uint64_t index = 0;
	uint32_t prev_pc = 0;
	
	while(it != end) {
		if(TR(*it).GetType() == InstructionHeader) {
			index++;
			prev_pc = IH(*it).GetPC();
		}
		
		if(TR(*it).GetType() == RegRead) {
			if(RR(*it).GetRegNum() == pc) printf("=> %u 0x%08x\n", index, prev_pc);
		}
		if(TR(*it).GetType() == RegWrite) {
			if(RW(*it).GetRegNum() == pc) printf("<= %u 0x%08x\n", index, prev_pc);
		}
		
		it++;
	}
	
	return 0;
}
