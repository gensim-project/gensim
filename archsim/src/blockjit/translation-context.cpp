/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/translation-context.h"

using namespace captive::arch::jit;


/**
 * Descriptor codes:
 *
 * X - No Operand
 * N - No Direction (e.g. constant only)
 * B - Input & Output
 * I - Input
 * O - Output
 */
struct captive::shared::insn_descriptor captive::shared::insn_descriptors[] = {
	{ .mnemonic = "invalid",	.format = "XXXXXX", .has_side_effects = false },
	{ .mnemonic = "verify",		.format = "NXXXXX", .has_side_effects = true },
	{ .mnemonic = "count",		.format = "NNXXXX", .has_side_effects = true },
	{ .mnemonic = "int_check",	.format = "XXXXXX", .has_side_effects = true },

	{ .mnemonic = "nop",		.format = "XXXXXX", .has_side_effects = false },
	{ .mnemonic = "trap",		.format = "XXXXXX", .has_side_effects = true },

	{ .mnemonic = "mov",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "cmov",		.format = "IIBXXX", .has_side_effects = false },
	{ .mnemonic = "ldpc",		.format = "OXXXXX", .has_side_effects = false },
	{ .mnemonic = "inc-pc",		.format = "IXXXXX", .has_side_effects = true },

	{ .mnemonic = "add",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "sub",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "imul",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "umul",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "udiv",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "sdiv",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "mod",		.format = "IBXXXX", .has_side_effects = false },

	{ .mnemonic = "shl",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "shr",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "sar",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "ror",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "rol",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "clz",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "popcnt",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "bswap",		.format = "IOXXXX", .has_side_effects = false },

	{ .mnemonic = "and",		.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "or",			.format = "IBXXXX", .has_side_effects = false },
	{ .mnemonic = "xor",		.format = "IBXXXX", .has_side_effects = false },

	{ .mnemonic = "cmp eq",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmp ne",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmp gt",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmp gte",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmp lt",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmp lte",	.format = "IIOXXX", .has_side_effects = false },

	{ .mnemonic = "mov sx",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "mov zx",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "mov trunc",	.format = "IOXXXX", .has_side_effects = false },

	{ .mnemonic = "ldreg",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "streg",		.format = "IIXXXX", .has_side_effects = true },
	{ .mnemonic = "ldmem",		.format = "NIIOXX", .has_side_effects = true },
	{ .mnemonic = "stmem",		.format = "NIIIXX", .has_side_effects = true },

	{ .mnemonic = "call",		.format = "NIIIII", .has_side_effects = true },
	{ .mnemonic = "jmp",		.format = "NXXXXX", .has_side_effects = true },
	{ .mnemonic = "branch",		.format = "INNXXX", .has_side_effects = true },
	{ .mnemonic = "ret",		.format = "XXXXXX", .has_side_effects = true },
	{ .mnemonic = "dispatch",	.format = "NNNNXX", .has_side_effects = true },

	{ .mnemonic = "scm",		.format = "IXXXXX", .has_side_effects = true },
	{ .mnemonic = "set_feature",.format = "NNXXXX", .has_side_effects = true },
	{ .mnemonic = "stdev",		.format = "IIIXXX", .has_side_effects = true },
	{ .mnemonic = "lddev",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "probedev",	.format = "IOXXXX", .has_side_effects = false },

	{ .mnemonic = "flush",		.format = "XXXXXX", .has_side_effects = true },
	{ .mnemonic = "flush itlb",	.format = "XXXXXX", .has_side_effects = true },
	{ .mnemonic = "flush dtlb",	.format = "XXXXXX", .has_side_effects = true },
	{ .mnemonic = "flush itlb",	.format = "IXXXXX", .has_side_effects = true },
	{ .mnemonic = "flush dtlb",	.format = "IXXXXX", .has_side_effects = true },

	{ .mnemonic = "adc flags",	.format = "IIIXXX", .has_side_effects = true },
	{ .mnemonic = "sbc flags",	.format = "IIIXXX", .has_side_effects = true },
	{ .mnemonic = "zn flags",	.format = "IXXXXX", .has_side_effects = true },

	{ .mnemonic = "barrier",	.format = "NXXXXX", .has_side_effects = true },
	{ .mnemonic = "exception",	.format = "IIXXXX", .has_side_effects = true },
	{ .mnemonic = "profile",	.format = "NXXXXX", .has_side_effects = true },

	{ .mnemonic = "cmps gt",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmps gte",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmps lt",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "cmps lte",	.format = "IIOXXX", .has_side_effects = false },

	{ .mnemonic = "fmul",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fdiv",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fadd",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fsub",		.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fsqrt",		.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fabs",		.format = "IOXXXX", .has_side_effects = false },

	{ .mnemonic = "fcmp_lt",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fcmp_lte",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fcmp_gt",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fcmp_gte",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fcmp_eq",	.format = "IIOXXX", .has_side_effects = false },
	{ .mnemonic = "fcmp_ne",	.format = "IIOXXX", .has_side_effects = false },

	{ .mnemonic = "fcvt_ui_to_f",	.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvt_f_to_ui",	.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvtt_f_to_ui",  .format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvt_si_to_f",	.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvt_f_to_si",	.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvtt_f_to_si",  .format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvt_s_to_d",	.format = "IOXXXX", .has_side_effects = false },
	{ .mnemonic = "fcvt_d_to_s",	.format = "IOXXXX", .has_side_effects = false },

	{ .mnemonic = "fctrl_setrnd",	.format = "IXXXXX", .has_side_effects = true },
	{ .mnemonic = "fctrl_getrnd",	.format = "OXXXXX", .has_side_effects = false },
	{ .mnemonic = "fctrl_setflush",	.format = "IXXXXX", .has_side_effects = true },
	{ .mnemonic = "fctrl_getflush",	.format = "OXXXXX", .has_side_effects = false },


	{ .mnemonic = "vaddi",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vaddf",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vsubi",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vsubf",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vmuli",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vmulf",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vori",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vandi",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vxori",	.format = "NIIOXX", .has_side_effects = false },

	{ .mnemonic = "vcmpeqi",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vcmpgti",	.format = "NIIOXX", .has_side_effects = false },
	{ .mnemonic = "vcmpgtei",	.format = "NIIOXX", .has_side_effects = false },

};

size_t captive::shared::num_descriptors = sizeof(captive::shared::insn_descriptors) / sizeof(captive::shared::insn_descriptors[0]);

TranslationContext::TranslationContext()
	: _ir_block_count(0), _ir_reg_count(0), _ir_insns(NULL), _ir_insn_count(0), _ir_insn_buffer_size(0)
{

}

void TranslationContext::clear()
{
	free_ir_buffer();
	_ir_block_count = 0;
	_ir_reg_count = 0;
	_ir_insn_count = 0;
	_ir_insn_buffer_size = 0;
}


TranslationContext::~TranslationContext()
{
	free(_ir_insns);
}

