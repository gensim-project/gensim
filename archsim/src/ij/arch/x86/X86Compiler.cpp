#include "ij/arch/x86/X86Compiler.h"
#include "ij/arch/x86/X86OutputBuffer.h"
#include "ij/arch/x86/X86Support.h"
#include "ij/IJIR.h"
#include "ij/ir/IRInstruction.h"
#include "util/LogContext.h"
#include "ij/ir/IRValue.h"

UseLogContext(LogIJ);
DeclareChildLogContext(LogX86, LogIJ, "X86");

#include <iostream>
#include <map>

using namespace archsim::ij;
using namespace archsim::ij::ir;
using namespace archsim::ij::arch::x86;

uint32_t X86Compiler::Compile(IJIR& ir, uint8_t* buffer)
{
	ir.Dump(std::cerr);

	std::map<IJIRBlock *, uint8_t *> block_ptrs;

	X86OutputBuffer ob(buffer);

	// Lower each IR block
	for (auto block : ir.GetBlocks()) {
		uint8_t *block_start_ptr = LowerBlock(ob, *block);
		if (!block_start_ptr) {
			LC_ERROR(LogX86) << "Block lowering failed for " << block->GetName();
			return 0;
		}

		block_ptrs[block] = block_start_ptr;
	}

	// Apply relocations

	return ob.GetSize();
}

uint8_t* X86Compiler::LowerBlock(X86OutputBuffer& buffer, IJIRBlock& block)
{
	LC_DEBUG2(LogX86) << "Lowering Block " << block.GetName() << " @ " << buffer.GetCurrent();

	uint8_t *block_start = buffer.GetCurrent();
	for (auto insn : block.GetInstructions()) {
		if (!LowerInstruction(buffer, *insn))
			return NULL;
	}

	return block_start;
}

bool X86Compiler::LowerInstruction(X86OutputBuffer& buffer, IRInstruction& insn)
{
	LC_DEBUG2(LogX86) << "Lowering Instruction @ " << buffer.GetCurrent();

	// Ensure all instruction operands have been allocated
	insn.EnsureOperandsAllocated();

	if (auto r = insn.as<IRRetInstruction>()) {
		return LowerRetInstruction(buffer, *r);
	} else if (auto r = insn.as<IRArithmeticInstruction>()) {
		return LowerArithmeticInstruction(buffer, *r);
	} else {
		LC_ERROR(LogX86) << "Unsupported IR instruction " << insn.GetTypeID() << " in backend";
		return false;
	}
}

bool X86Compiler::LowerRetInstruction(X86OutputBuffer& buffer, IRRetInstruction& insn)
{
	if (insn.HasValue()) {
		switch(insn.GetValue()->GetAllocationClass()) {
			case IRValue::Constant:
				if (insn.GetValue()->GetAllocationData() == 0)
					buffer._xor(X86Register::RAX, X86Register::RAX);
				else
					buffer.mov(ValueToImmediate(insn.GetValue()), X86Register::GetSizedRegister(X86Register::GPR_A, insn.GetValue()->GetSize()));
				break;
			case IRValue::Register:
				buffer.mov(ValueToRegister(insn.GetValue()), X86Register::GetSizedRegister(X86Register::GPR_A, insn.GetValue()->GetSize()));
				break;
			default:
				assert(false);
		}
	}

	buffer.ret();
	return true;
}

bool X86Compiler::LowerArithmeticInstruction(X86OutputBuffer& buffer, IRArithmeticInstruction& insn)
{
	// SRC must be allocated, and DEST must not be allocated to a constant
	assert(insn.GetSource().IsAllocated() && insn.GetDestination().IsNotConstAllocated());

	// SRC and DEST must not both be allocated to the stack
	assert(!(insn.GetSource().GetAllocationClass() == IRValue::Stack && insn.GetSource().GetAllocationClass() == IRValue::Stack));

	switch (insn.GetOperation()) {
		case IRArithmeticInstruction::Add:
			buffer.int3();
			return true;
		default:
			LC_ERROR(LogX86) << "Unsupported arithmetic instruction " << std::dec << insn.GetOperation() << " in backend";
			return false;
	}
}

const X86Register& X86Compiler::ValueToRegister(ir::IRValue *val)
{
	assert((val->GetAllocationClass() == IRValue::Register) && "Value not allocated to register");
	assert(false && "Unsupported value size for register allocation");
	__builtin_unreachable();
}

const X86Immediate X86Compiler::ValueToImmediate(ir::IRValue *val)
{
	assert((val->GetAllocationClass() == IRValue::Constant) && "Value not allocated to constant");
	return X86Immediate(val->GetSize(), val->GetAllocationData());
}
