#include "libtrace/RecordTypes.h"
#include "libtrace/RecordFile.h"

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <map>
#include <set>

using namespace libtrace;

TraceRecord TR(Record r) {
	return *(TraceRecord*)&r;
}

InstructionHeaderRecord IH(Record r) {
	return *(InstructionHeaderRecord*)&r;
}
InstructionCodeRecord IC(Record r) {
	return *(InstructionCodeRecord*)&r;
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
	FILE *f1 = fopen(argv[1], "r");
	FILE *f2 = fopen(argv[2], "r");
	
	if(!f1 || !f2) {
		return 1;
	}
	
	RecordFile rf1 (f1);
	RecordFile rf2 (f2);
	
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
	
	std::map<uint32_t, uint32_t> file1_mem, file2_mem;
	
	// make sure we are at an instruction header
	while(TR(*it1).GetType() != InstructionHeader) it1++;
	while(TR(*it2).GetType() != InstructionHeader) it2++;

	std::set<uint32_t> ignore_codes { 0x180cf93, 0x1841f93, 0x1830f91, 0x1824f9c, 0x1820f91, 0x1831f92, 0x1892f93, 0x1812f93, 0x18c0f91, 0x1821f93, 0x180ef9c, 0x1801f92, 0x181cf90, 0x18c1f92, 0x1802f93, 0x1842f93, 0x180cf91, 0x1a32f96, 0x1821f95, 0x1830f92, 0x181cf92, 0x1841f92 };

	uint64_t counter = 0;

	// scan until PC divergence
	while(true) {
	
		file1_mem.clear();
		file2_mem.clear();
		
		ctr1++;
		ctr2++;
		counter++;
		
		if((counter % 10000000) == 0) printf("%lu...\n", counter);

		if(IH(*it1).GetPC() != IH(*it2).GetPC()) {
			printf("Divergence detected at instruction %lu %lu\n", ctr1, ctr2);
			return 1;
		}
		
		it1++;
		it2++;
		uint32_t code = IC(*it1).GetIR();
		bool check = true;
		             
//		if((code & 0xff00ff0) == 0x1800f90) check = false;
//		else if((code & 0xff00ff0) == 0x1a00f90) check = false;
//		else if(ignore_codes.count(code & 0x0fffffff)) check = false;

		do {
			if(TR(*it1).GetType() == MemReadAddr) {
				MemReadAddrRecord mra = MRA(*it1++);
				MemReadDataRecord mrd = MRD(*it1++);
				
				for(uint32_t i = 0; i < mra.GetWidth(); ++i) file1_mem.insert({mra.GetAddress() + i, (mrd.GetData() >> (i*8)) & 0xff});
				//file1_mem.insert({mra.GetAddress(), mrd.GetData()});
				
			} else if(TR(*it1).GetType() == MemWriteAddr) {
				MemWriteAddrRecord mwa = MWA(*it1++);
				MemWriteDataRecord mwd = MWD(*it1++);
				
				for(uint32_t i = 0; i < mwa.GetWidth(); ++i) file1_mem.insert({mwa.GetAddress() + i, (mwd.GetData() >> (i*8)) & 0xff});
				//file1_mem.insert({mwa.GetAddress(), mwd.GetData()});
				
			} else {
				it1++;
			}
		} while(TR(*it1).GetType() != InstructionHeader);

		do {
			if(TR(*it2).GetType() == MemReadAddr) {
				MemReadAddrRecord mra = MRA(*it2++);
				MemReadDataRecord mrd = MRD(*it2++);
				
				for(uint32_t i = 0; i < mra.GetWidth(); ++i) file2_mem.insert({mra.GetAddress() + i, (mrd.GetData() >> (i*8)) & 0xff});
				
				//file2_mem.insert({mra.GetAddress(), mrd.GetData()});
				
			} else if(TR(*it2).GetType() == MemWriteAddr) {
				MemWriteAddrRecord mwa = MWA(*it2++);
				MemWriteDataRecord mwd = MWD(*it2++);
				
				for(uint32_t i = 0; i < mwa.GetWidth(); ++i) file2_mem.insert({mwa.GetAddress() + i, (mwd.GetData() >> (i*8)) & 0xff});
				
				//file2_mem.insert({mwa.GetAddress(), mwd.GetData()});
				
			} else {
				it2++;
			}
		} while(TR(*it2).GetType() != InstructionHeader);

		if(check) {
			if(file1_mem.size() && file2_mem.size()) {
				if(file1_mem != file2_mem) {
					printf("Memory divergence detected at instruction %lu %lu\n", ctr1, ctr2);
					
					std::set<uint32_t> addrs;
					for(auto i : file1_mem) addrs.insert(i.first);
					for(auto i : file2_mem) addrs.insert(i.first);
					
					for(auto i : addrs) {
						printf("%08x %08x %08x\n", i, file1_mem[i], file2_mem[i]);
					}
					
					return 1;
				}
			}
		}

	}
	
}
