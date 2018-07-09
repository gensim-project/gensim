/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/x86/X86Decoder.h"

extern "C" {
#include <xed/xed-interface.h>
}

using namespace archsim;
using namespace archsim::arch::x86;

int X86Decoder::DecodeInstr(Address addr, int mode, MemoryInterface& interface)
{
	// read 15 bytes
	uint8_t data[15];
	interface.Read(addr, data, 15);

	// pass data to XED
	xed_state_t dstate;
	xed_decoded_inst_t xedd;
	xed_error_enum_t xed_error;

	xed_tables_init();
	xed_state_zero(&dstate);
	dstate.mmode = XED_MACHINE_MODE_LONG_64;

	xed_decoded_inst_zero_set_mode(&xedd, &dstate);
	xed_decoded_inst_set_input_chip(&xedd, XED_CHIP_HASWELL);

	xed_error = xed_decode(&xedd, XED_REINTERPRET_CAST(const xed_uint8_t*, data), 15);

	auto category = xed_decoded_inst_get_category(&xedd);
	auto iclass = xed_decoded_inst_get_iclass(&xedd);

	Instr_Length = xed_decoded_inst_get_length(&xedd);

	printf("%p %u %u\n", addr.Get(), category, iclass);

	return 0;
}
