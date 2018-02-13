#include "libtrace/RecordTypes.h"
#include "libtrace/RecordStream.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

using namespace libtrace;

TraceRecord TR(Record r) { return *(TraceRecord*)&r; }
InstructionHeaderRecord IHR(Record r) { return *(InstructionHeaderRecord*)&r; }

int main(int argc, char **argv)
{
	FILE *f;
	if(!strcmp(argv[1], "-")) f = stdin;
	else f = fopen(argv[1], "r");
	
	if(!f) {
		fprintf(stderr, "Could not open file\n");
		return 1;
	}
	
	RecordStream rf(f);
	
	uint32_t prev_pc = 0xffffffff;
	uint64_t count = 0;
	
	bool print = true;
	while(rf.good()) {
		Record r = rf.next();
		if(TR(r).GetType() == InstructionHeader) {
			count++;
			if((count % 10000000) == 0) fprintf(stderr, "Uniq'd %lu instructions\n", count);
			
			if(IHR(r).GetPC() == prev_pc) {
				print = false;
			} else {
				print = true;
				prev_pc = IHR(r).GetPC();
			}
		}
		
		if(TR(r).GetType() == InstructionHeader || TR(r).GetType() == InstructionCode) {
			if(print) fwrite(&r, sizeof(r), 1, stdout);
		} else {
			fwrite(&r, sizeof(r), 1, stdout);	
		}
	}
	
	return 0;
}
