/*
 * LowerMemoryCache.cpp
 *
 *  Created on: 23 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/lowering/Finalisation.h"
#include "blockjit/block-compiler/block-compiler.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"
#include "translate/jit_funs.h"

#include "abi/memory/system/CacheBasedSystemMemoryModel.h"

using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

extern "C" {
	uint32_t blkCacheRead8Fallback();
	uint32_t blkCacheRead16Fallback();
	uint32_t blkCacheRead32Fallback();

	uint32_t blkCacheWrite8Fallback();
	uint32_t blkCacheWrite16Fallback();
	uint32_t blkCacheWrite32Fallback();
}

// Cache entry layout:
// Virtual address tag (32 bits)
// Physical address (20 bits) + Flags (12 bits)
// Data (64 bits)

class LowerMemCacheReadFinaliser : public captive::arch::jit::lowering::Finalisation
{
public:
	LowerMemCacheReadFinaliser(X86Encoder &encoder, uint32_t interface_id, uint32_t jump_in, uint32_t alignment_in, uint32_t return_target, uint32_t access_size, const X86Register &dest_reg) :
		_encoder(encoder), interface_id_(interface_id), jump_in_(jump_in), nonaligned_in_(alignment_in), return_target_(return_target), access_size_(access_size), destination_(dest_reg)
	{

	}

	virtual ~LowerMemCacheReadFinaliser() {}
	bool Finalise(captive::arch::jit::lowering::LoweringContext &ctx)
	{

		// First, fill in the relocation from the memory instruction
		*(uint32_t*)(Encoder().get_buffer() + jump_in_) = Encoder().current_offset() - jump_in_ - 4;
		if(nonaligned_in_)
			*(uint32_t*)(Encoder().get_buffer() + nonaligned_in_) = Encoder().current_offset() - nonaligned_in_ - 4;

		// We need to fix the stack, since the ABI requires that (%rsp+8) is 16
		// byte aligned.
		if(ctx.GetStackFrameSize() % 16 == 8) {
			Encoder().sub(8, REG_RSP);
		}
		Encoder().push(REG_RDI); // thread
		Encoder().push(REG_RSI); // address
		Encoder().push(REG_RDX); // interface
		
		Encoder().mov(REG_R12, BLKJIT_ARG0(8));
		Encoder().mov(REG_R15, BLKJIT_ARG1(8));
		Encoder().mov(interface_id_, BLKJIT_ARG2(8));

		// Now, emit the memory instruction fallback
		switch(access_size_) {
			case 1:
				Encoder().call((void*)blkCacheRead8Fallback, BLKJIT_RETURN(8));
				break;
			case 2:
				Encoder().call((void*)blkCacheRead16Fallback, BLKJIT_RETURN(8));
				break;
			case 4:
				Encoder().call((void*)blkCacheRead32Fallback, BLKJIT_RETURN(8));
				break;
			default:
				assert(false);
		}
		
		Encoder().pop(REG_RDX); // interface
		Encoder().pop(REG_RSI); // address
		Encoder().pop(REG_RDI); // thread

		if(ctx.GetStackFrameSize() % 16 == 8) {
			Encoder().add(8, REG_RSP);
		}
		
		if(destination_ != REG_RIZ) {
			Encoder().mov(BLKJIT_RETURN(access_size_), destination_);
		}

		// Finally, emit a jump back to the IR lowering after the memory instruction
		Encoder().jmp_offset(return_target_ - Encoder().current_offset() - 5);

		return true;
	}

	X86Encoder &Encoder()
	{
		return _encoder;
	}

private:
	X86Encoder &_encoder;

	// The offset of the relocation for the 32 bit jump into this finalisation
	uint32_t jump_in_;
	
	// The ID of the interface of this memory access
	uint32_t interface_id_;

	// The offset of the relocation for the jump into this finalisation, for non-aligned accesses
	uint32_t nonaligned_in_;

	// The offset of the instruction to return to once the memory fallback is complete
	uint32_t return_target_;

	// The size in bytes of this memory access
	uint32_t access_size_;

	// The register to use as the destination of the memory read
	const X86Register &destination_;
};

class LowerMemCacheWriteFinaliser : public captive::arch::jit::lowering::Finalisation
{
public:
	LowerMemCacheWriteFinaliser(X86Encoder &encoder, uint32_t interface_id, uint32_t jump_in, uint32_t alignment_in, uint32_t return_target, uint32_t access_size, const X86Register &data) :
		_encoder(encoder), interface_id_(interface_id), _jump_in(jump_in), _nonaligned_in(alignment_in), _return_target(return_target), _access_size(access_size), destination_reg_(data), destination_stack_(REG_RIZ), destination_is_reg_(true)
	{

	}

	LowerMemCacheWriteFinaliser(X86Encoder &encoder, uint32_t interface_id, uint32_t jump_in, uint32_t alignment_in, uint32_t return_target, uint32_t access_size, const X86Memory &data) :
		_encoder(encoder), interface_id_(interface_id), _jump_in(jump_in), _nonaligned_in(alignment_in), _return_target(return_target), _access_size(access_size), destination_reg_(REG_RIZ), destination_stack_(data), destination_is_reg_(false)
	{

	}

	virtual ~LowerMemCacheWriteFinaliser() {}
	bool Finalise(captive::arch::jit::lowering::LoweringContext &ctx)
	{

		// First, fill in the relocation from the memory instruction
		*(uint32_t*)(Encoder().get_buffer() + _jump_in) = Encoder().current_offset() - _jump_in - 4;
		if(_nonaligned_in)
			*(uint32_t*)(Encoder().get_buffer() + _nonaligned_in) = Encoder().current_offset() - _nonaligned_in - 4;


		// thread
		// interface
		// address
		// data
		
		Encoder().push(REG_RDI);
		Encoder().push(REG_RSI);
		Encoder().push(REG_RDX);
		Encoder().push(REG_RCX);
		
		Encoder().mov(X86Memory::get(REG_R12), BLKJIT_ARG0(8));
		Encoder().mov(BLKJIT_ARG1(8), BLKJIT_ARG2(8));
		Encoder().mov(interface_id_, BLKJIT_ARG1(8));

		if(destination_is_reg_) {
			Encoder().mov(destination_reg_, REGS_RCX(_access_size));
		} else {
			Encoder().mov(X86Memory::get(REG_RSP, destination_stack_.displacement + 0x20), REGS_RCX(_access_size));
		}

		if(ctx.GetStackFrameSize() % 16 != 8) {
			Encoder().sub(8, REG_RSP);
		}
		
		// We need to fix the stack, since the ABI requires that (%rsp+8) is 16
		// byte aligned.
		Encoder().mov(interface_id_, BLKJIT_ARG1(8));

		switch(_access_size) {
			case 1:
				Encoder().call((void*)blkCacheWrite8Fallback, BLKJIT_RETURN(8));
				break;
			case 2:
				Encoder().call((void*)blkCacheWrite16Fallback, BLKJIT_RETURN(8));
				break;
			case 4:
				Encoder().call((void*)blkCacheWrite32Fallback, BLKJIT_RETURN(8));
				break;
		}
		
		if(ctx.GetStackFrameSize() % 16 != 8) {
			Encoder().add(8, REG_RSP);
		}
		Encoder().pop(REG_RCX);
		Encoder().pop(REG_RDX);
		Encoder().pop(REG_RSI);
		Encoder().pop(REG_RDI);
		
		
		// Finally, emit a jump back to the IR lowering after the memory instruction
		Encoder().jmp_offset(_return_target - Encoder().current_offset() - 5);

		return true;
	}

	X86Encoder &Encoder()
	{
		return _encoder;
	}

private:
	bool _is_read;

	X86Encoder &_encoder;

	// The offset of the relocation for the 32 bit jump into this finalisation
	uint32_t _jump_in;
	
	uint32_t interface_id_;

	// The offset of the relocation for the jump into this finalisation, for non-aligned accesses
	uint32_t _nonaligned_in;

	// The offset of the instruction to return to once the memory fallback is complete
	uint32_t _return_target;

	// The size in bytes of this memory access
	uint32_t _access_size;

	// The register to use as the destination of the memory read
	bool destination_is_reg_;
	const X86Register &destination_reg_;

	const X86Memory destination_stack_;
};

bool LowerReadMemCache::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *interface = &insn->operands[0];
	const IROperand *offset = &insn->operands[1];
	const IROperand *disp = &insn->operands[2];
	const IROperand *dest = &insn->operands[3];

	assert(disp->is_constant());
	assert(interface->is_constant());
	uint32_t interface_id = interface->value;
	
	// liveness is broken in some situations apparently
	uint32_t live_regs = 0xffffffff;//host_liveness.live_out[ir_idx];
	// Don't save/restore the destination reg
	if(dest->is_alloc_reg()) live_regs &= ~(1 << dest->alloc_data);

	//TODO: support these things coming in on the stack
	assert(disp->is_constant());


	// We can have the situation where dest is not allocated because the intervening register write has been eliminated
	const auto &dest_reg = dest->is_allocated() && !dest->is_alloc_stack() ? GetLoweringContext().register_from_operand(dest) : GetLoweringContext().get_temp(0, dest->size);

	if(disp->value != 0) {
		if(offset->is_alloc_reg()) {
			Encoder().lea(X86Memory::get(GetLoweringContext().register_from_operand(offset, 4), disp->value), BLKJIT_ARG1(4));
		} else if(offset->is_alloc_stack()) {
			Encoder().mov(GetLoweringContext().stack_from_operand(offset), BLKJIT_ARG1(4));
			Encoder().add(disp->value, BLKJIT_ARG1(4));
		} else {
			assert(false);
		}
	} else {
		if(offset->is_alloc_reg())
			Encoder().mov(GetLoweringContext().register_from_operand(offset, 4), BLKJIT_ARG1(4));
		else
			Encoder().mov(GetLoweringContext().stack_from_operand(offset), BLKJIT_ARG1(4));
	}

	// Back up address
	Encoder().mov(BLKJIT_ARG1(4), BLKJIT_RETURN(4));
	Encoder().mov(BLKJIT_ARG1(4), BLKJIT_ARG2(4));

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

	using archsim::abi::memory::SMMCache;
	using archsim::abi::memory::SMMCacheEntry;

	// Calculate cache entry index - calculated as page index % cache size
	// Mask out TLB index, then shift right to correct position
	uint32_t mask = SMMCache::kCacheSize-1;
	// XXX ARM HAX
	mask <<= 12;
	Encoder().andd(mask, BLKJIT_ARG1(4));
	Encoder().shr(8, BLKJIT_ARG1(4));
	Encoder().add(X86Memory::get(BLKJIT_CPUSTATE_REG, GetLoweringContext().GetThread()->GetStateBlock().GetBlockOffset("smm_read_cache")), BLKJIT_ARG1(8));

	// Check the tag
	Encoder().andd(~archsim::translate::profile::RegionArch::PageMask, BLKJIT_ARG2(4));
	Encoder().cmp(BLKJIT_ARG2(4), X86Memory::get(BLKJIT_ARG1(8), 0));

	// If not equal, jump to fallback
	uint32_t fallback_offset;
	Encoder().jne_reloc(fallback_offset);

	// add tlb addend to calculated address
	Encoder().add(X86Memory::get(BLKJIT_ARG1(8), 8), BLKJIT_RETURN(8));

	// perform memory access
	if(dest->is_allocated()) {
		if(dest->is_alloc_reg()) {
			// If the destination is a register, load directly into it
			Encoder().mov(X86Memory::get(BLKJIT_RETURN(8)), dest_reg);
			GetLoweringContext().RegisterFinalisation(new LowerMemCacheReadFinaliser(Encoder(), interface_id, fallback_offset, alignment_offset, Encoder().current_offset(), dest->size, dest_reg));
		} else if(dest->is_alloc_stack()) {
			// Otherwise, load via a temporary register
			Encoder().mov(X86Memory::get(BLKJIT_RETURN(8)), dest_reg);
			GetLoweringContext().RegisterFinalisation(new LowerMemCacheReadFinaliser(Encoder(), interface_id, fallback_offset, alignment_offset, Encoder().current_offset(), dest->size, dest_reg));
			Encoder().mov(dest_reg, GetLoweringContext().stack_from_operand(dest));
		} else {
			assert(false);
		}
	}

	insn++;
	return true;
}

bool LowerWriteMemCache::Lower(const captive::shared::IRInstruction *&insn)
{
	const IROperand *interface = &insn->operands[0];
	const IROperand *value = &insn->operands[1];
	const IROperand *disp = &insn->operands[2];
	const IROperand *offset = &insn->operands[3];

	assert(interface->is_constant());
	assert(disp->is_constant());

	assert(offset->is_vreg());
	assert(disp->is_constant());

	uint32_t interface_id = interface->value;
	
	Encoder().ensure_extra_buffer(128);
	//State register will be dealt with by the shunt

	if(offset->is_alloc_reg()) {
		Encoder().mov(GetLoweringContext().get_allocable_register(offset->alloc_data, 4), BLKJIT_ARG0(4));
	} else if(offset->is_alloc_stack()) {
		Encoder().mov(GetLoweringContext().stack_from_operand(offset), BLKJIT_ARG0(4));
	}
	if(disp->is_alloc_reg()) {
		Encoder().add(GetLoweringContext().get_allocable_register(disp->alloc_data, 4), BLKJIT_ARG0(4));
	} else if(disp->is_constant()) {
		if(disp->value != 0)
			Encoder().add(disp->value, BLKJIT_ARG0(4));
	} else {
		assert(false);
	}

	Encoder().mov(BLKJIT_ARG0(4), BLKJIT_ARG1(4));
	Encoder().mov(BLKJIT_ARG0(4), BLKJIT_ARG2(4));

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
			Encoder().test(mask, BLKJIT_ARG0(1));
			Encoder().jne_reloc(alignment_offset);
		}
	}

	using archsim::abi::memory::SMMCache;
	using archsim::abi::memory::SMMCacheEntry;

	// Calculate cache entry index - calculated as page index % cache size
	// Mask out TLB index, then shift right to correct position
	uint32_t mask = SMMCache::kCacheSize-1;
	// XXX ARM HAX
	mask <<= 12;
	Encoder().andd(mask, BLKJIT_ARG0(4));
	Encoder().shr(8, BLKJIT_ARG0(4));
	Encoder().add(X86Memory::get(BLKJIT_CPUSTATE_REG, GetLoweringContext().GetThread()->GetStateBlock().GetBlockOffset("smm_write_cache")), BLKJIT_ARG0(8));

	// Check the tag
	Encoder().andd(~archsim::translate::profile::RegionArch::PageMask, BLKJIT_ARG2(4));
	Encoder().cmp(BLKJIT_ARG2(4), X86Memory::get(BLKJIT_ARG0(8), 0));

	// If not equal, jump to fallback
	uint32_t fallback_offset;
	Encoder().jne_reloc(fallback_offset);

	// calculate address to write to

	// calculate page offset of memory access
	Encoder().add(X86Memory::get(BLKJIT_ARG0(8), 8), BLKJIT_ARG1(8));

	// perform memory access
	assert(value->is_vreg());
	if(value->is_alloc_reg()) {
		Encoder().mov(GetLoweringContext().get_allocable_register(value->alloc_data, value->size), X86Memory::get(BLKJIT_ARG1(8)));
		GetLoweringContext().RegisterFinalisation(new LowerMemCacheWriteFinaliser(Encoder(), interface_id, fallback_offset, alignment_offset, Encoder().current_offset(), value->size, GetLoweringContext().get_allocable_register(value->alloc_data, value->size)));
	} else if(value->is_alloc_stack()) {
		Encoder().mov(GetLoweringContext().stack_from_operand(value), BLKJIT_ARG0(value->size));
		Encoder().mov(BLKJIT_ARG0(value->size), X86Memory::get(BLKJIT_ARG1(8)));
		GetLoweringContext().RegisterFinalisation(new LowerMemCacheWriteFinaliser(Encoder(), interface_id, fallback_offset, alignment_offset, Encoder().current_offset(), value->size, GetLoweringContext().stack_from_operand(value)));
	}

	insn++;
	return true;
}

bool LowerReadUserMemCache::Lower(const captive::shared::IRInstruction *&insn)
{
	UNIMPLEMENTED;
}

bool LowerWriteUserMemCache::Lower(const captive::shared::IRInstruction *&insn)
{
	UNIMPLEMENTED;
}
