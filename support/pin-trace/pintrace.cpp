/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <pin.H>
#include <libtrace/TraceSource.h>
#include <libtrace/TraceSink.h>

libtrace::TraceSource *trace_source;
libtrace::TraceSink *trace_sink;

#define BANK_INVALID ((uint32_t)0xff)
#define INDEX_INVALID ((uint32_t)0xff)


uint32_t getBankOf(REG reg)
{
	switch(reg) {
		case REG_RAX:
		case REG_RBX:
		case REG_RCX:
		case REG_RDX:
		case REG_RDI:
		case REG_RSI:
		case REG_RSP:
		case REG_RBP:
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			return 0;

		case REG_EAX:
		case REG_EBX:
		case REG_ECX:
		case REG_EDX:
		case REG_EDI:
		case REG_ESI:
		case REG_ESP:
		case REG_EBP:
		case REG_R8D:
		case REG_R9D:
		case REG_R10D:
		case REG_R11D:
		case REG_R12D:
		case REG_R13D:
		case REG_R14D:
		case REG_R15D:
			return 1;

		case REG_AX:
		case REG_BX:
		case REG_CX:
		case REG_DX:
		case REG_DI:
		case REG_SI:
		case REG_SP:
		case REG_BP:
		case REG_R8W:
		case REG_R9W:
		case REG_R10W:
		case REG_R11W:
		case REG_R12W:
		case REG_R13W:
		case REG_R14W:
		case REG_R15W:
			return 2;

		case REG_AL:
		case REG_BL:
		case REG_CL:
		case REG_DL:
		case REG_DIL:
		case REG_SIL:
		case REG_SPL:
		case REG_BPL:
		case REG_R8B:
		case REG_R9B:
		case REG_R10B:
		case REG_R11B:
		case REG_R12B:
		case REG_R13B:
		case REG_R14B:
		case REG_R15B:
			return 3;

		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			return 7;
		default:
			return BANK_INVALID;
	}
}

uint32_t getIndexOf(REG reg)
{
	switch(reg) {
		case REG_RAX:
		case REG_EAX:
		case REG_AX:
		case REG_AL:
		case REG_AH:
			return 0;
		case REG_RBX:
		case REG_EBX:
		case REG_BX:
		case REG_BL:
		case REG_BH:
			return 3;
		case REG_RCX:
		case REG_ECX:
		case REG_CX:
		case REG_CL:
		case REG_CH:
			return 1;
		case REG_RDX:
		case REG_EDX:
		case REG_DX:
		case REG_DL:
		case REG_DH:
			return 2;
		case REG_RSI:
		case REG_ESI:
		case REG_SI:
		case REG_SIL:
			return 6;
		case REG_RDI:
		case REG_EDI:
		case REG_DI:
		case REG_DIL:
			return 7;

		case REG_RSP:
		case REG_ESP:
		case REG_SP:
		case REG_SPL:
			return 4;

		case REG_RBP:
		case REG_EBP:
		case REG_BP:
		case REG_BPL:
			return 5;

		case REG_R8:
		case REG_R8D:
		case REG_R8W:
		case REG_R8B:
			return 8;
		case REG_R9:
		case REG_R9D:
		case REG_R9W:
		case REG_R9B:
			return 9;
		case REG_R10:
		case REG_R10D:
		case REG_R10W:
		case REG_R10B:
			return 10;
		case REG_R11:
		case REG_R11D:
		case REG_R11W:
		case REG_R11B:
			return 11;
		case REG_R12:
		case REG_R12D:
		case REG_R12W:
		case REG_R12B:
			return 12;
		case REG_R13:
		case REG_R13D:
		case REG_R13W:
		case REG_R13B:
			return 13;
		case REG_R14:
		case REG_R14D:
		case REG_R14W:
		case REG_R14B:
			return 14;
		case REG_R15:
		case REG_R15D:
		case REG_R15W:
		case REG_R15B:
			return 15;


		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			return reg - (int)REG_XMM0;

		default:
			return INDEX_INVALID;
	}
	return 0;
}

bool shouldTrace(REG reg)
{
	return getBankOf(reg) != BANK_INVALID && getIndexOf(reg) != INDEX_INVALID;
}

VOID trace_insn(void *inst_ptr, uint32_t inst_data)
{
	trace_source->Trace_Insn((uint64_t)inst_ptr, inst_data, 0, 0, 0, 0);
}

VOID trace_read_reg(uint32_t bank, uint32_t index, REG reg, CONTEXT* context)
{
	union {
		uint64_t rval;
		char rdata[16];
	};
	PIN_GetContextRegval(context, reg, (uint8_t*)&rdata);
	int width;

	switch(bank) {
		case 0:
			width = 8;
			break;
		case 1:
			width = 4;
			break;
		case 2:
			width = 2;
			break;
		case 3:
			width = 1;
			break;
		case 7:
			width = 16;
			break;

	}
	trace_source->Trace_Bank_Reg_Read(true, bank, index, rdata, width);
}

VOID trace_mem_read(uint64_t addr, uint8_t width)
{
	uint64_t data;
	switch(width) {
		case 1:
			data = *(uint8_t*)addr;
			break;
		case 2:
			data = *(uint16_t*)addr;
			break;
		case 4:
			data = *(uint32_t*)addr;
			break;
		case 8:
			data = *(uint64_t*)addr;
			break;
		case 16:
			trace_source->Trace_Mem_Read(0, (uint64_t)addr, *(uint64_t*)addr, width);
			trace_source->Trace_Mem_Read(0, (uint64_t)addr+8, *(uint64_t*)(addr+8), width);
			return;
		default:
			//		printf("Unknown memory access size %u\n", width);
			//		abort();
			data = 0;
			break;
	}

	trace_source->Trace_Mem_Read(0, (uint64_t)addr, (uint64_t)data, width);
}

VOID trace_mem_write_prep(uint64_t addr, uint8_t width)
{

}

VOID trace_mem_write(uint64_t addr, uint8_t width)
{
	trace_source->Trace_Mem_Write(0, (uint64_t)addr, (uint64_t)0, width);
}

VOID trace_insn_end(void *inst_ptr)
{
	trace_source->Trace_End_Insn();
}

static uint32_t eax_leaf, ecx_leaf;

VOID my_cpuid_before(CONTEXT *ctx)
{
	//record EAX and ECX to figure out what leaf we're using for later
	eax_leaf = PIN_GetContextReg(ctx, REG_RAX);
	ecx_leaf = PIN_GetContextReg(ctx, REG_RCX);
}

VOID my_cpuid_after(CONTEXT *ctx)
{
	printf("CPUID %x %x\n", eax_leaf, ecx_leaf);

	uint32_t eax, ebx, ecx, edx;
	eax = 0x0;
	ebx = 0x0;
	ecx = 0x0;
	edx = 0x0;

	switch(eax_leaf) {
		case 0:
			ebx = 'G' << 0 | 'e' << 8 | 'n' << 16 | 'u' << 24;
			ecx = 'n' << 0 | 't' << 8 | 'e' << 16 | 'l' << 24;
			edx = 'i' << 0 | 'n' << 8 | 'e' << 16 | 'I' << 24;
			eax = 0x16;
			break;
		case 1:
			eax = 0x000506e3;

			ecx = 0;
			ecx |= (0 << 0); // SSE3
			ecx |= (0 << 1); // PCLMULQDQ
			ecx |= (0 << 2); // 64 bit debug store
			ecx |= (0 << 3); // MONITOR and MWAIT
			ecx |= (0 << 4); // CPL qualified debug store
			ecx |= (0 << 5); // VMX
			ecx |= (0 << 6); // SMX
			ecx |= (0 << 7); // EST
			ecx |= (0 << 8); // TM2
			ecx |= (0 << 9); // SSSE3
			ecx |= (0 << 10); // CNXT-ID
			ecx |= (0 << 11); // Silicon Debug Interface
			ecx |= (0 << 12); // FMA3
			ecx |= (0 << 13); // CMPXCHG16B
			ecx |= (0 << 14); // XTPR
			ecx |= (0 << 15); // PDCM
			ecx |= (0 << 16); // RESERVED
			ecx |= (0 << 17); // PCID
			ecx |= (0 << 18); // DCA
			ecx |= (0 << 19); // SSE4.1
			ecx |= (0 << 20); // SSE4.2
			ecx |= (0 << 21); // X2APIC
			ecx |= (0 << 22); // MOVBE
			ecx |= (1 << 23); // POPCNT
			ecx |= (0 << 24); // TSC-DEADLINE
			ecx |= (0 << 25); // AES
			ecx |= (1 << 26); // XSAVE
			ecx |= (0 << 27); // OSXSAVE
			ecx |= (0 << 28); // AVX
			ecx |= (0 << 29); // F16C
			ecx |= (0 << 30); // RDRAND
			ecx |= (0 << 31); // HYPERVISOR

			edx = 0;
			edx |= (1 << 0); // FPU
			edx |= (0 << 1); // VME
			edx |= (0 << 2); // DE
			edx |= (0 << 3); // PSE
			edx |= (0 << 4); // TSC
			edx |= (0 << 5); // MSR
			edx |= (0 << 6); // PAE
			edx |= (0 << 7); // MCE
			edx |= (1 << 8); // CX8
			edx |= (0 << 9); // APIC
			edx |= (0 << 10); // RESERVED
			edx |= (0 << 11); // SEP
			edx |= (0 << 12); // MTRR
			edx |= (0 << 13); // PGE
			edx |= (0 << 14); // MCA
			edx |= (1 << 15); // CMOV
			edx |= (0 << 16); // PAT
			edx |= (0 << 17); // PSE-36
			edx |= (0 << 18); // PSN
			edx |= (1 << 19); // CFSH
			edx |= (0 << 20); // RESERVED
			edx |= (0 << 21); // DS
			edx |= (0 << 22); // ACPI
			edx |= (1 << 23); // MMX
			edx |= (0 << 24); // FXSR
			edx |= (1 << 25); // SSE
			edx |= (1 << 26); // SSE2
			edx |= (0 << 27); // SS
			edx |= (0 << 28); // HTT
			edx |= (0 << 29); // TM
			edx |= (0 << 30); // IA64
			edx |= (0 << 31); // PBE

			ebx = 0; //ebx;
			break;
		case 2:
			eax = 0x01060a01;
			ebx = 0;
			ecx = 0;
			edx = 0;
			break;
		case 7:
			eax = 0;
			ebx = 0;
			ecx = 0;
			edx = 0;
			break;
		default:
			printf("Unknown CPUID leaf %x!\n", eax_leaf);
			break;
	}

	PIN_SetContextReg(ctx, REG_RAX, eax);
	PIN_SetContextReg(ctx, REG_RBX, ebx);
	PIN_SetContextReg(ctx, REG_RCX, ecx);
	PIN_SetContextReg(ctx, REG_RDX, edx);

	PIN_ExecuteAt(ctx);
}


VOID Trace(TRACE trace, VOID *v)
{
	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
		for ( INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
			char ins_data[4] = {0};
			void *ins_addr = (void*)INS_Address(ins);
			uint32_t size = MIN(4, INS_Size(ins));
			for(int i = 0; i < size; ++i) {
				ins_data[i] = ((uint8_t*)ins_addr)[i];
			}

			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_insn), IARG_INST_PTR, IARG_UINT32, *(uint32_t*)ins_data, IARG_END);

			// trace register accesses
			for(uint32_t i = 0; i < INS_MaxNumRRegs(ins); ++i) {
				REG reg = INS_RegR(ins, i);
				if(shouldTrace(reg)) {
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_read_reg), IARG_UINT32, getBankOf(reg), IARG_UINT32, getIndexOf(reg), IARG_UINT32, reg, IARG_CONTEXT, IARG_END);
				}
			}

			// trace memory access (assume only one)
			uint32_t memops = INS_MemoryOperandCount(ins);
			for(uint32_t i = 0; i < memops; ++i) {
				if(INS_MemoryOperandIsRead(ins, i)) {
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_mem_read), IARG_MEMORYOP_EA, i, IARG_MEMORYREAD_SIZE, IARG_END);
				} else {
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_mem_write), IARG_MEMORYOP_EA, i, IARG_MEMORYWRITE_SIZE, IARG_END);
				}

			}

			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_insn_end), IARG_END);



			if(INS_Opcode(ins) == XED_ICLASS_CPUID) {
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(my_cpuid_before), IARG_CONTEXT, IARG_END);
				INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(my_cpuid_after), IARG_CONTEXT, IARG_END);
			}
		}
	}
}

VOID Fini(INT32 code, VOID *v)
{
	trace_sink->Flush();
	delete trace_sink;
}

int main(int argc, char **argv)
{
	if( PIN_Init(argc,argv) ) {
		printf("Failed to initialise\n");
		return -1;
	}

	FILE *f = fopen("trace.out", "w");
	trace_sink = new libtrace::BinaryFileTraceSink(f);

	trace_source = new libtrace::TraceSource(1024);
	trace_source->SetSink(trace_sink);

	PIN_AddFiniFunction(Fini, 0);

	TRACE_AddInstrumentFunction(Trace, 0);
	PIN_StartProgram();

	return 0;
}
