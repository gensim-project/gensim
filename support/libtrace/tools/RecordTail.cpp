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
	fprintf(stderr, "Skipping %lu instructions\n", count);
	
	while(count && rf.good()) {
		Record r = rf.next();
		TraceRecord *tr = (TraceRecord *)&r;
		
		if(tr->GetType() == InstructionHeader) {
			count--;
			if(!(count % 10000000)) fprintf(stderr, "%lu remaining...\n",count);
		}
		
	}
	if(!rf.good()) {
		fprintf(stderr, "Reached end of stream before reaching instruction count\n");
		return 1;
	}
	
	// advance to the next instruction header
	while(true && rf.good()) {
		Record r = rf.peek();
		TraceRecord *tr = (TraceRecord *)&r;
		
		if(tr->GetType() == InstructionHeader) break;
		rf.next();
	}
	if(!rf.good()) {
		fprintf(stderr, "Reached end of stream before reaching instruction head\n");
		return 1;
	}
	
	std::vector<Record> buffer;
	
	while(rf.good()) {
		Record r = rf.next();
		
		buffer.push_back(r);
		if(buffer.size() == 1024) {
			fwrite(buffer.data(), sizeof(Record), 1024, stdout);
			buffer.clear();
		}
		
	}
	
	fprintf(stderr, "Reached end of stream.\n");
	
	fwrite(buffer.data(), sizeof(Record), buffer.size(), stdout);
	buffer.clear();
	
	return 0;
}
