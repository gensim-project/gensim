/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"
#include "util/LogContext.h"

UseLogContext(LogBlockJit)

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

bool LowerVCmpEQI::Lower(const captive::shared::IRInstruction*& insn)
{
	const IROperand &width = insn->operands[0]; // VECTOR width, i.e. number of elements
	const IROperand &lhs = insn->operands[1];
	const IROperand &rhs = insn->operands[2];
	const IROperand &dest = insn->operands[3];

	UNIMPLEMENTED;

	insn++;
	return true;
}
