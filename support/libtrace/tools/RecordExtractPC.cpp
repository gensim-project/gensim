#include "libtrace/RecordTypes.h"
#include "libtrace/RecordFile.h"

#include <cstdlib>
#include <cstdio>

using namespace libtrace;

int main(int argc, char **argv)
{
	FILE *f = fopen(argv[1], "r");
	RecordFile rf(f);
	
	auto it = rf.begin();
	auto end = rf.end();
	
	while(it != end) {
		Record r = *it;
		TraceRecord *tr = (TraceRecord *)&r;
		if(tr->GetType() == InstructionHeader) {
			printf("%08x\n", tr->GetData32());
		}
		it++;
	}
	
	return 0;
}
