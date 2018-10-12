/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/RecordTypes.h"
#include "libtrace/RecordFile.h"

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <map>
#include <set>

using namespace libtrace;

TraceRecord TR(Record r)
{
	return *(TraceRecord*)&r;
}

InstructionHeaderRecord IH(Record r)
{
	return *(InstructionHeaderRecord*)&r;
}
InstructionCodeRecord IC(Record r)
{
	return *(InstructionCodeRecord*)&r;
}

MemReadAddrRecord MRA(Record r)
{
	return *(MemReadAddrRecord*)&r;
}

MemReadDataRecord MRD(Record r)
{
	return *(MemReadDataRecord*)&r;
}

MemWriteAddrRecord MWA(Record r)
{
	return *(MemWriteAddrRecord*)&r;
}

MemWriteDataRecord MWD(Record r)
{
	return *(MemWriteDataRecord*)&r;
}

class Memory
{
public:
	void Insert(uint64_t address, uint8_t byte)
	{
		storage_.push_back({address, byte});
	}

	void Clear()
	{
		storage_.clear();
	}

	bool operator!=(const Memory &other)
	{
		return !(operator==(other));
	}

	bool operator==(const Memory &other)
	{
		for(auto me : storage_) {
			for(auto them : other.storage_) {
				if(me.first == them.first) {
					if(me.second != them.second) {
						return false;
					}
				}
			}
		}
		return true;
	}

private:
	std::vector<std::pair<uint64_t, uint8_t>> storage_;

};

void UpdateMemoryMap(Memory &mem, libtrace::TraceRecordPacket &addr_record, libtrace::TraceRecordPacket &data_record)
{
	assert(addr_record.GetExtensions().size() <= 1);
	assert(data_record.GetExtensions().size() <= 1);

	uint64_t full_addr = MWA(addr_record.GetRecord()).GetAddress();
	if(addr_record.GetExtensions().size() == 1) {
		full_addr |= ((uint64_t)addr_record.GetExtensions().at(0).GetData32()) << 32;
	}

	auto mwd = MWD(data_record.GetRecord());
	uint64_t full_data = MWD(addr_record.GetRecord()).GetData();
	if(data_record.GetExtensions().size() == 1) {
		full_data |= ((uint64_t)data_record.GetExtensions().at(0).GetData32()) << 32;
	}

	uint8_t *data_array = (uint8_t*)&full_data;

	int width = MWA(addr_record.GetRecord()).GetWidth();

	for(int i = 0; i < width; ++i) {
		mem.Insert(full_addr + i, data_array[i]);
	}
}

int main(int argc, char **argv)
{
	std::ifstream f1(argv[1]);
	std::ifstream f2(argv[2]);

	char *buffer1 = (char*)malloc(1024 * 1024);
	char *buffer2 = (char*)malloc(1024 * 1024);

	f1.rdbuf()->pubsetbuf(buffer1, 1024 * 1024);
	f2.rdbuf()->pubsetbuf(buffer2, 1024 * 1024);

	if(!f1 || !f2) {
		return 1;
	}

	RecordFileInputStream rf1 (f1);
	RecordFileInputStream rf2 (f2);

	TracePacketStreamAdaptor tpsa1(&rf1);
	TracePacketStreamAdaptor tpsa2(&rf2);

	uint64_t start1 = 0;
	uint64_t start2 = 0;

	if(argc == 4) {
		start1 = atoi(argv[3]);
		start2 = start1;
	}
	if(argc == 5) {
		start1 = atoi(argv[3]);
		start2 = atoi(argv[4]);
	}

	uint64_t ctr1 = start1, ctr2 = start2;

	// seek to starts
	while(start1) {
		if(tpsa1.Get().GetRecord().GetType() == InstructionHeader) start1--;
	}

	while(start2) {
		if(tpsa2.Get().GetRecord().GetType() == InstructionHeader) start1--;
	}

	Memory file1_mem, file2_mem;

	// make sure we are at an instruction header
	while(tpsa1.Peek().GetRecord().GetType() != InstructionHeader) {
		tpsa1.Get();
	}
	while(tpsa2.Peek().GetRecord().GetType() != InstructionHeader) {
		tpsa2.Get();
	}

	assert(tpsa1.Peek().GetRecord().GetType() == InstructionHeader);
	assert(tpsa2.Peek().GetRecord().GetType() == InstructionHeader);

	uint64_t counter = 0;

	// scan until PC divergence
	while(tpsa1.Good() && tpsa2.Good()) {

		file1_mem.Clear();
		file2_mem.Clear();

		ctr1++;
		ctr2++;
		counter++;

		if((counter % 10000000) == 0) printf("%lu...\n", counter);

		// Both streams should be pointing to instruction headers
		TraceRecordPacket tp1 = tpsa1.Get();
		TraceRecordPacket tp2 = tpsa2.Get();

		assert(tp1.GetRecord().GetType() == InstructionHeader);
		assert(tp2.GetRecord().GetType() == InstructionHeader);

		if(IH(tp1.GetRecord()).GetPC() != IH(tp2.GetRecord()).GetPC()) {
			printf("PC Divergence detected at instruction %lu %lu\n", ctr1, ctr2);
			return 1;
		}

		while(tpsa1.Good() && tpsa1.Peek().GetRecord().GetType() != InstructionHeader) {
			TraceRecordPacket trp = tpsa1.Get();

			if(trp.GetRecord().GetType() == libtrace::MemWriteAddr) {
				TraceRecordPacket data = tpsa1.Get();
				assert(data.GetRecord().GetType() == libtrace::MemWriteData);

				UpdateMemoryMap(file1_mem, trp, data);
			}
		}

		while(tpsa2.Good() && tpsa2.Peek().GetRecord().GetType() != InstructionHeader) {
			TraceRecordPacket trp = tpsa2.Get();

			if(trp.GetRecord().GetType() == libtrace::MemWriteAddr) {
				TraceRecordPacket data = tpsa2.Get();
				assert(data.GetRecord().GetType() == libtrace::MemWriteData);

				UpdateMemoryMap(file2_mem, trp, data);
			}
		}

		if(file1_mem != file2_mem) {
			printf("Memory divergence detected at instruction %lu %lu\n", ctr1, ctr2);
			return 2;
		}
	}

	return 0;
}
