/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/RecordFile.h"
#include "libtrace/InstructionPrinter.h"

#include <iostream>
#include <cstdio>
#include <set>

using namespace libtrace;

TraceRecord TR(const Record &r)
{
	return *(TraceRecord*)&r;
}

InstructionHeaderRecord IH(Record &r)
{
	return *(InstructionHeaderRecord*)&r;
}

class CutDescriptor
{
public:
	CutDescriptor(FILE *f)
	{
		Load(f);
	}

	bool ShouldCut(uint64_t pc)
	{
		if(cut_pcs_.count(pc)) {
			return true;
		} else {
			for(auto i : cut_ranges_) {
				if(pc >= i.first && pc <= i.second) {
					return true;
				}
			}
		}
		return false;
	}

private:
	void Load(FILE *f)
	{
		uint64_t pc;
		uint64_t pc2;
		while(true) {
			int i = fscanf(f, "%li - %li\n", &pc, &pc2);
			if(i == 1) {
				cut_pcs_.insert(pc);
			} else if(i == 2) {
				cut_ranges_.insert({pc, pc2});
			} else {
				break;
			}
		}
	}

	std::set<uint64_t> cut_pcs_;
	std::set<std::pair<uint64_t, uint64_t>> cut_ranges_;
};

int main(int argc, char **argv)
{
	if(argc != 3) {
		fprintf(stderr, "Usage: %s [record file] [cut descriptor file]\n", argv[0]);
		return 1;
	}

	FILE *record_file = fopen(argv[1], "r");
	if(!record_file) {
		perror("Could not open file");
		return 1;
	}

	FILE *cut_file = fopen(argv[2], "r");
	if(!cut_file) {
		perror("Could not open file");
		return 1;
	}

	CutDescriptor cd(cut_file);

	RecordFile rf (record_file);
	RecordBufferStreamAdaptor rbsa (&rf);

	while(rbsa.Good()) {
		auto record = rbsa.Peek();

		assert(IH(record).GetType() == InstructionHeader);

		if(cd.ShouldCut(IH(record).GetPC())) {
			// skip to next instruction header
			do {
				rbsa.Get();
			} while(rbsa.Good() && TR(rbsa.Peek()).GetType() != InstructionHeader);
		} else {
			// write out until next instruction header
			do {
				record = rbsa.Get();
				fwrite((void*)&record, sizeof(record), 1, stdout);
			} while(rbsa.Good() && TR(rbsa.Peek()).GetType() != InstructionHeader);
		}

	}

	return 0;
}