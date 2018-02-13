/*
 * LowerMemoryGeneric.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */


#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/blockjit-abi.h"
#include "translate/jit_funs.h"

#include "gensim/gensim_processor_state.h"
#include "blockjit/blockjit-shunts.h"
#include "blockjit/block-compiler/lowering/Finalisation.h"

uint32_t blkFnFallbackSlot;

using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;
using namespace captive::shared;

class LowerMemReadFunctionFinaliser : public captive::arch::jit::lowering::Finalisation
{
public:
	LowerMemReadFunctionFinaliser(X86Encoder &encoder, uint32_t jump_in, uint32_t alignment_in, uint32_t return_target, uint32_t access_size)
		: _encoder(encoder), _jump_in(jump_in), _alignment_in(alignment_in), _return_target(return_target), _access_size(access_size)
	{

	}
	virtual ~LowerMemReadFunctionFinaliser() {}

	bool Finalise(captive::arch::jit::lowering::LoweringContext &ctx) override
	{
		// First, fill in jump targets for alignment fallback and fault fallback
		*(uint32_t*)(Encoder().get_buffer() + _jump_in) = Encoder().current_offset() - _jump_in - 4;
		if(_alignment_in) {
			*(uint32_t*)(Encoder().get_buffer() + _alignment_in) = Encoder().current_offset() - _alignment_in - 4;
		}

		// Now, emit fallback code
		Encoder().mov(BLKJIT_CPUSTATE_REG, BLKJIT_ARG0(8));
		Encoder().mov(BLKJIT_RETURN(4), BLKJIT_ARG1(4));
		switch(_access_size) {
			case 1:
				Encoder().call((void*)blkFnRead8Fallback, BLKJIT_RETURN(8));
				break;
			case 2:
				Encoder().call((void*)blkFnRead16Fallback, BLKJIT_RETURN(8));
				break;
			case 4:
				Encoder().call((void*)blkFnRead32Fallback, BLKJIT_RETURN(8));
				break;
		}

		// Finally, emit a jump back to the IR lowering after the memory instruction
		Encoder().jmp_offset(_return_target - Encoder().current_offset() - 5);

		return true;
	}

	X86Encoder &Encoder()
	{
		return _encoder;
	}

private:
	uint32_t _jump_in, _alignment_in, _return_target, _access_size;
	X86Encoder &_encoder;
};


class LowerMemWriteFunctionFinaliser : public captive::arch::jit::lowering::Finalisation
{
public:
	LowerMemWriteFunctionFinaliser(X86Encoder &encoder, uint32_t jump_in, uint32_t alignment_in, uint32_t return_target, const IROperand &value, LowerWriteMemFunction &write, X86LoweringContext::stack_map_t &stack_map)
		: _encoder(encoder), _jump_in(jump_in), _alignment_in(alignment_in), _return_target(return_target), _value(value), _write(write), _stack_map(stack_map)
	{

	}
	virtual ~LowerMemWriteFunctionFinaliser() {}

	bool Finalise(captive::arch::jit::lowering::LoweringContext &ctx) override
	{
		// First, fill in jump targets for alignment fallback and fault fallback
		*(uint32_t*)(Encoder().get_buffer() + _jump_in) = Encoder().current_offset() - _jump_in - 4;
		if(_alignment_in) {
			*(uint32_t*)(Encoder().get_buffer() + _alignment_in) = Encoder().current_offset() - _alignment_in - 4;
		}

		// Fault detected, so call fallback
		_write.GetCompiler().load_state_field(0, BLKJIT_ARG0(8));
		Encoder().mov(BLKJIT_RETURN(4), BLKJIT_ARG1(4));

		_write.GetCompiler().encode_operand_function_argument(&_value, BLKJIT_ARG2(_value.size), _stack_map);
		switch(_value.size) {
			case 1:
				Encoder().call((void*)cpuWrite8, BLKJIT_RETURN(8));
				break;
			case 2:
				Encoder().call((void*)cpuWrite16, BLKJIT_RETURN(8));
				break;
			case 4:
				Encoder().call((void*)cpuWrite32, BLKJIT_RETURN(8));
				break;
		}

		// Finally, emit a jump back to the IR lowering after the memory instruction
		Encoder().jmp_offset(_return_target - Encoder().current_offset() - 5);

		return true;
	}

	X86Encoder &Encoder()
	{
		return _encoder;
	}

private:
	uint32_t _jump_in, _alignment_in, _return_target, _access_size;
	X86Encoder &_encoder;
	LowerWriteMemFunction &_write;
	const IROperand &_value;
	X86LoweringContext::stack_map_t _stack_map;
};

#define SAVE_EVERYTHING
bool LowerReadMemFunction::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *offset = &insn->operands[0];
	const IROperand *disp = &insn->operands[1];
	const IROperand *dest = &insn->operands[2];

	assert(disp->is_constant());

	// Registers used:
	// BLKJIT_ARG0
	// BLKJIT_ARG1
	// BLKJIT_ARG2 (RDX)
	// BLKJIT_RETURN
	// RAX!
	// RDX

#ifdef SAVE_EVERYTHING

	// liveness is broken in some situations apparently
	uint32_t live_regs = 0xffffffff;//host_liveness.live_out[ir_idx];
	// Don't save/restore the destination reg
	if(dest->is_alloc_reg()) live_regs &= ~(1 << dest->alloc_data);

	GetCompiler().emit_save_reg_state(2, GetStackMap(), GetIsStackFixed(), live_regs);

#else
//	if(dest_reg != REGS_RAX(dest_reg.size)) {
	{
		Encoder().push(REG_RAX);
//		Encoder().push(REG_RBX);
//		Encoder().push(REG_RCX);
//		Encoder().push(REG_R8);
//		Encoder().push(REG_R9);
//		Encoder().push(REG_R10);
//		Encoder().push(REG_R11);
	}
#endif

	// Compute final address
	Encoder().mov(BLKJIT_CPUSTATE_REG, BLKJIT_ARG0(8));
	const X86Register *offset_reg = nullptr;
	if(offset->is_alloc_reg()) {
		offset_reg = &GetCompiler().register_from_operand(offset, 4);
	} else if(offset->is_alloc_stack()) {
		offset_reg = &BLKJIT_TEMPS_1(4);
		Encoder().mov(GetCompiler().stack_from_operand(offset), BLKJIT_TEMPS_1(4));
	} else {
		assert(false);
	}

	assert(offset_reg != nullptr);

	if(disp->value != 0) {
		Encoder().lea(X86Memory::get(*offset_reg, disp->value), BLKJIT_ARG1(4));

		// Back up address in case we need to fall back
		Encoder().mov(BLKJIT_ARG1(4), BLKJIT_RETURN(4));
	} else {
		Encoder().mov(*offset_reg, BLKJIT_ARG1(4));

		// Back up address in case we need to fall back
		Encoder().mov(*offset_reg, BLKJIT_RETURN(4));
	}

	uint32_t alignment_offset = 0;
	if(archsim::options::MemoryCheckAlignment) {
		uint8_t mask = 0;
		switch(dest->size) {
			case 1:
				mask = 0;
				break;
			case 2:
				mask = 1;
				break;
			case 4:
				mask = 3;
				break;
			default:
				assert(false);
				break;
		}
		if(mask) {
			Encoder().test(mask, BLKJIT_ARG1(1));
			Encoder().jne_reloc(alignment_offset);
		}
	}

	// Load function pointer
	GetCompiler().load_state_field(gensim::CpuStateOffsets::CpuState_smm_read_cache, BLKJIT_TEMPS_2(8));
	Encoder().mov(BLKJIT_ARG1(4), REG_EAX);
	Encoder().shr(12, REG_EAX);

	// Call function
	Encoder().call(X86Memory::get(BLKJIT_TEMPS_2(8), REG_RAX, 8));
	// SI & DI clobbered, AX & DX return regs

	// Check return value (0 = OK, nonzero = fault)
	Encoder().test(REG_EAX, REG_EAX);
	uint32_t jmp;
	Encoder().jne_reloc(jmp);

	// Fallback might return (if it's a device access)
	// Fallback returns value in RAX, fallback memory location in RDX

	uint32_t return_target = Encoder().current_offset();

#ifdef SAVE_EVERYTHING
	GetCompiler().emit_restore_reg_state(2, GetStackMap(), GetIsStackFixed(), live_regs);
#else
//	if(dest_reg != REGS_RAX(dest_reg.size))
	{
//		Encoder().pop(REG_R11);
//		Encoder().pop(REG_R10);
//		Encoder().pop(REG_R9);
//		Encoder().pop(REG_R8);

//		Encoder().pop(REG_RCX);
//		Encoder().pop(REG_RBX);
		Encoder().pop(REG_RAX);
	}
#endif

	if(dest->is_alloc_reg()) {
		const X86Register &dest_reg = dest->is_allocated() ? GetCompiler().get_allocable_register(dest->alloc_data, dest->size) : BLKJIT_TEMPS_0(dest->size);

		if(dest_reg != BLKJIT_TEMPS_0(dest->size)) {
			Encoder().mov(X86Memory::get(REG_RDX), dest_reg);
		}
	} else if(dest->is_alloc_stack()) {
		Encoder().mov(X86Memory::get(REG_RDX), BLKJIT_TEMPS_0(dest->size));
		Encoder().mov(BLKJIT_TEMPS_0(dest->size), GetCompiler().stack_from_operand(dest));
	} else {
		// do nothing: this might happen if a load is performed but the result is ignored
	}

	GetLoweringContext().RegisterFinalisation(new LowerMemReadFunctionFinaliser(Encoder(), jmp, alignment_offset, return_target, dest->size));

	insn++;
	return true;
}

bool LowerWriteMemFunction::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *value = &insn->operands[0];
	const IROperand *disp = &insn->operands[1];
	const IROperand *offset = &insn->operands[2];

	assert(disp->is_constant());

	// Registers used:
	// BLKJIT_ARG0
	// BLKJIT_ARG1
	// BLKJIT_ARG2 (RDX)
	// BLKJIT_RETURN
	// RAX!
	// RDX

	GetCompiler().emit_save_reg_state(3, GetStackMap(), GetIsStackFixed());

	auto &stack_map = GetStackMap();

	// Compute final address
	Encoder().mov(BLKJIT_CPUSTATE_REG, BLKJIT_ARG0(8));
	const X86Register *offset_reg = nullptr;
	if(offset->is_alloc_reg()) {
		offset_reg = &GetCompiler().register_from_operand(offset, 4);
	} else if(offset->is_alloc_stack()) {
		offset_reg = &BLKJIT_TEMPS_1(4);
		Encoder().mov(GetCompiler().stack_from_operand(offset), BLKJIT_TEMPS_1(4));
	} else {
		assert(false);
	}

	assert(offset_reg != nullptr);

	if(disp->value != 0) {
		Encoder().lea(X86Memory::get(*offset_reg, disp->value), BLKJIT_ARG1(4));
	} else {
		Encoder().mov(*offset_reg, BLKJIT_ARG1(4));
	}

	// Back up address in case we need to fall back
	Encoder().mov(BLKJIT_ARG1(4), BLKJIT_RETURN(4));

	uint32_t alignment_offset = 0;
	if(archsim::options::MemoryCheckAlignment) {
		uint8_t mask = 0;
		switch(value->size) {
			case 1:
				mask = 0;
				break;
			case 2:
				mask = 1;
				break;
			case 4:
				mask = 3;
				break;
			default:
				assert(false);
				break;
		}
		if(mask) {
			Encoder().test(mask, BLKJIT_ARG1(1));
			Encoder().jne_reloc(alignment_offset);
		}
	}

	// Load function pointer
	GetCompiler().load_state_field(gensim::CpuStateOffsets::CpuState_smm_write_cache, BLKJIT_TEMPS_2(8));
	Encoder().mov(BLKJIT_ARG1(4), REG_EAX);
	Encoder().shr(12, REG_EAX);

	// Call function
	Encoder().call(X86Memory::get(BLKJIT_TEMPS_2(8), REG_RAX, 8));
	// SI & DI clobbered, AX & DX return regs

	// Check return value (0 = OK, nonzero = fault)
	Encoder().test(REG_EAX, REG_EAX);
	uint32_t jmp;
	Encoder().jne_reloc(jmp);

	if(value->is_alloc_reg()) {
		const X86Register &dest_reg = GetCompiler().get_allocable_register(value->alloc_data, value->size);
		GetCompiler().encode_operand_function_argument(value, dest_reg, GetStackMap());
		Encoder().mov(dest_reg, X86Memory::get(REG_RDX));
	} else if(value->is_alloc_stack()) {
		Encoder().mov(GetCompiler().stack_from_operand(value), REGS_RAX(value->size));
		Encoder().mov(REGS_RAX(value->size), X86Memory::get(REG_RDX));
	}

	uint32_t return_target = Encoder().current_offset();

	GetCompiler().emit_restore_reg_state(3, GetStackMap(), GetIsStackFixed());

	GetLoweringContext().RegisterFinalisation(new LowerMemWriteFunctionFinaliser(Encoder(), jmp, alignment_offset, return_target, *value, *this, stack_map));

	insn++;
	return true;
}

bool LowerReadUserMemFunction::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *offset = &insn->operands[0];
	const IROperand *dest = &insn->operands[1];

	assert(offset->is_alloc_reg());

	// Registers used:
	// BLKJIT_ARG0
	// BLKJIT_ARG1
	// BLKJIT_ARG2 (RDX)
	// BLKJIT_RETURN
	// RAX!
	// RDX

	assert(!dest->is_alloc_stack());
	const X86Register &dest_reg = dest->is_allocated() ? GetCompiler().get_allocable_register(dest->alloc_data, dest->size) : BLKJIT_TEMPS_0(dest->size);

	// liveness is broken in some situations apparently
	uint32_t live_regs = 0xffffffff;//host_liveness.live_out[ir_idx];
	// Don't save/restore the destination reg
	if(dest->is_alloc_reg()) live_regs &= ~(1 << dest->alloc_data);

	GetCompiler().emit_save_reg_state(2, GetStackMap(), GetIsStackFixed(), live_regs);

	// Compute final address
	Encoder().mov(BLKJIT_CPUSTATE_REG, BLKJIT_ARG0(8));
	Encoder().mov(GetCompiler().register_from_operand(offset, 4), BLKJIT_ARG1(4));

	// Back up address in case we need to fall back
	Encoder().mov(BLKJIT_ARG1(4), BLKJIT_RETURN(4));

	// Load function pointer
	GetCompiler().load_state_field(gensim::CpuStateOffsets::CpuState_smm_read_user_cache, BLKJIT_TEMPS_2(8));
	Encoder().mov(BLKJIT_ARG1(4), REG_EAX);
	Encoder().shr(12, REG_EAX);

	// Call function
	Encoder().call(X86Memory::get(BLKJIT_TEMPS_2(8), REG_RAX, 8));
	// SI & DI clobbered, AX & DX return regs

	// Check return value (0 = OK, nonzero = fault)
	Encoder().test(REG_EAX, REG_EAX);
	uint32_t jmp;
	Encoder().je_reloc(jmp);

	// Fault detected, so call fallback
	Encoder().mov(BLKJIT_CPUSTATE_REG, BLKJIT_ARG0(8));
	Encoder().mov(BLKJIT_RETURN(4), BLKJIT_ARG1(4));
	switch(dest->size) {
		case 1:
			Encoder().call((void*)blkFnRead8UserFallback, BLKJIT_RETURN(8));
			break;
		case 2:
			Encoder().call((void*)blkFnRead16UserFallback, BLKJIT_RETURN(8));
			break;
		case 4:
			Encoder().call((void*)blkFnRead32UserFallback, BLKJIT_RETURN(8));
			break;
	}

	*(uint32_t*)(Encoder().get_buffer() + jmp) = Encoder().current_offset() - jmp - 4;

	if(dest_reg != BLKJIT_TEMPS_0(dest->size)) {
		Encoder().mov(X86Memory::get(REG_RDX), dest_reg);
	}

	GetCompiler().emit_restore_reg_state(2, GetStackMap(), GetIsStackFixed(), live_regs);

	insn++;
	return true;
}

bool LowerWriteUserMemFunction::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *value = &insn->operands[0];
	const IROperand *offset = &insn->operands[1];

	GetCompiler().emit_save_reg_state(3, GetStackMap(), GetIsStackFixed());

	switch(value->size) {
		case 1:
			Encoder().mov((uint64_t)cpuWrite8User, BLKJIT_RETURN(8));
			break;
		case 2:
			assert(false);
			break;
		case 4:
			Encoder().mov((uint64_t)cpuWrite32User, BLKJIT_RETURN(8));
			break;
		default:
			assert(false);
	}

	GetCompiler().load_state_field(0, REG_RDI);
	GetCompiler().encode_operand_function_argument(offset, REG_RSI, GetStackMap());
	GetCompiler().encode_operand_function_argument(value, REG_RDX, GetStackMap());

	Encoder().call(BLKJIT_RETURN(8));

	GetCompiler().emit_restore_reg_state(3, GetStackMap(), GetIsStackFixed());

	insn++;
	return true;
}
