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

MemReadAddrRecord MRA(Record r) {
	return *(MemReadAddrRecord*)&r;
}

MemReadDataRecord MRD(Record r) {
	return *(MemReadDataRecord*)&r;
}

MemWriteAddrRecord MWA(Record r) {
	return *(MemWriteAddrRecord*)&r;
}

MemWriteDataRecord MWD(Record r) {
	return *(MemWriteDataRecord*)&r;
}



int main(int argc, char **argv)
{
	FILE *f = fopen(argv[1], "r");
	RecordFile rf (f);
	
	uint32_t seek_addr = strtol(argv[2], NULL, 16);
	
	auto it = rf.begin();
	auto end = rf.end();
	
	while(it != end) 
	{
		if(TR(*it).GetType() == MemReadAddr) {
			if(seek_addr == MRA(*it).GetAddress()) {
				it++;
				printf("%u\n", MRD(*it).GetData());
			} else {
				it++;
			}
		} else {
			it++;
		}
	}

	return 0;
}
