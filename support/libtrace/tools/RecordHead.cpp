#include "libtrace/RecordTypes.h"
#include "libtrace/RecordStream.h"

#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace libtrace;

int main(int argc, char **argv)
{
	FILE *f;
	
	if(!strcmp(argv[1], "-")) f = stdin;
	else f = fopen(argv[1], "r");	
	
	if(!f) return 1;
	
	RecordStream rf(f);
	
	uint64_t count = strtol(argv[2], NULL, 0);
	
	std::vector<Record> buffer;
	while(count > 0 && rf.good()) {
		Record r = rf.next();
		TraceRecord *tr = (TraceRecord *)&r;
		
		if(tr->GetType() == InstructionHeader) count--;
		
		buffer.push_back(r);
		if(buffer.size() == 1024) {
			fwrite(buffer.data(), sizeof(Record), 1024, stdout);
			buffer.clear();
		}
	}
	
	fwrite(buffer.data(), sizeof(Record), buffer.size(), stdout);
	buffer.clear();
	
	return 0;
}
