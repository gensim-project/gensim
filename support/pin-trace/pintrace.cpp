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

		default:
			return INDEX_INVALID;
	}
	return 0;
}

bool shouldTrace(REG reg)
{
	return getBankOf(reg) != BANK_INVALID && getIndexOf(reg) != INDEX_INVALID;
}

VOID trace_insn(void *inst_ptr)
{
	trace_source->Trace_Insn((uint64_t)inst_ptr, (uint32_t)0, 0, 0, 0, 0);
}

VOID trace_read_reg(uint32_t bank, uint32_t index, uint64_t value)
{
	trace_source->Trace_Bank_Reg_Read(true, bank, index, (uint64_t)value);
}

VOID trace_insn_end(void *inst_ptr)
{
	trace_source->Trace_End_Insn();
}

VOID Trace(TRACE trace, VOID *v)
{
	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
		for ( INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_insn), IARG_INST_PTR, IARG_END);

			// trace register accesses
			for(uint32_t i = 0; i < INS_MaxNumRRegs(ins); ++i) {
				REG reg = INS_RegR(ins, i);
				if(shouldTrace(reg)) {
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_read_reg), IARG_UINT32, getBankOf(reg), IARG_UINT32, getIndexOf(reg), IARG_REG_VALUE, reg, IARG_END);
				}
			}

			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trace_insn_end), IARG_END);
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
