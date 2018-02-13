/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <libtrace/RecordFile.h>

using namespace libtrace;

int main(int argc, char **argv) {
	char *filename = argv[1];
	
	RecordFile file(fopen(filename, "r"));
	
	RecordBufferStreamAdaptor rbsa(&file);
	while(rbsa.Good()) {
		Record r = rbsa.Get();
		
		TraceRecord tr = *(TraceRecord*)&r;
		
		switch(tr.GetType()) {
			case Unknown: printf("Unknown"); break;
			case InstructionHeader : printf("Instruction Header"); break;
			case InstructionCode : printf("Instruction Code"); break;
			case RegRead: printf("Reg Read"); break;
			case RegWrite: printf("Reg Write"); break;
			case BankRegRead: printf("Bank Reg Read"); break;
			case BankRegWrite: printf("Bank Reg Write"); break;
			case MemReadAddr: printf("Mem Read Addr"); break;
			case MemReadData: printf("Mem Read Data"); break;
			case MemWriteAddr: printf("Mem Write Addr"); break;
			case MemWriteData: printf("Mem Write Data"); break;
			
			case DataExtension: printf("Data Extension"); break;
			
			default: printf("Unhandled %u", tr.GetType()); break;
		}
		
		printf("\n");
	}
	
	return 0;
}