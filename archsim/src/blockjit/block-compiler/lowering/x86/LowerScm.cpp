#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"

#include "gensim/gensim_processor_state.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

bool LowerScm::Lower(const captive::shared::IRInstruction *&insn)
{
	auto &value = insn->operands[0];

	GetCompiler().lea_state_field(gensim::CpuStateOffsets::CpuState_isa_mode, BLKJIT_TEMPS_0(8));
	if(value.is_constant()) {
		Encoder().mov1(value.value, X86Memory::get(BLKJIT_TEMPS_0(8)));
	} else if(value.is_alloc_reg()) {
		Encoder().mov(GetCompiler().register_from_operand(&value), X86Memory::get(BLKJIT_TEMPS_0(8)));
	} else {
		assert(false);
	}

	insn++;
	return true;
}