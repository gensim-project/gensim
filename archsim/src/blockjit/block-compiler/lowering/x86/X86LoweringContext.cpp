/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * X86LoweringContext.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/InstructionLowerer.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/block-compiler/lowering/x86/X86BlockjitABI.h"

#include "util/LogContext.h"

UseLogContext(LogLower);

using namespace captive::arch::jit::lowering;
using namespace captive::arch::jit::lowering::x86;
using namespace captive::shared;

X86LoweringContext::X86LoweringContext(uint32_t stack_frame_size, encoder_t &encoder, const archsim::ArchDescriptor &arch, const archsim::StateBlockDescriptor &sbd, const wutils::vbitset<> &used_regs) : MCLoweringContext(stack_frame_size, arch, sbd), _encoder(encoder), _stack_fixed(false), used_phys_regs(used_regs)
{
	int i = 0;
#define ASSIGN_REGS(x) assign(i++, x(8), x(4), x(2), x(1))
	ASSIGN_REGS(REGS_RAX);
	ASSIGN_REGS(REGS_RBX);
	ASSIGN_REGS(REGS_RCX);
	ASSIGN_REGS(REGS_R8);
	ASSIGN_REGS(REGS_R9);
	ASSIGN_REGS(REGS_R10);
	ASSIGN_REGS(REGS_R11);
	ASSIGN_REGS(REGS_R13);
	ASSIGN_REGS(REGS_R14);

#undef ASSIGN_REGS
}

X86LoweringContext::~X86LoweringContext()
{
}


#define LowerType(X) Lower ## X Singleton_Lower ## X;
#define LowerTypeTS(X) Lower ## X Singleton_Lower ## X;
#include "blockjit/block-compiler/lowering/x86/LowerTypes.h"
#undef LowerType

NOPInstructionLowerer Singleton_LowerNop;

bool X86LoweringContext::Prepare(const TranslationContext &ctx)
{
	using captive::shared::IRInstruction;

	AddLowerer(IRInstruction::NOP, &Singleton_LowerNop);
	AddLowerer(IRInstruction::BARRIER, &Singleton_LowerNop);

#define A(X, Y) AddLowerer(X, &Singleton_Lower ## Y)
	A(IRInstruction::SET_CPU_MODE, Scm);
	A(IRInstruction::SET_CPU_FEATURE, SetCpuFeature);
	A(IRInstruction::INTCHECK, IntCheck);
	A(IRInstruction::TRAP, Trap);
	A(IRInstruction::CMOV, CMov);
	A(IRInstruction::MOV, Mov);
	A(IRInstruction::UDIV, UDiv);
	A(IRInstruction::SDIV, SDiv);
	A(IRInstruction::IMUL, IMul);
	A(IRInstruction::MUL, UMul);
	A(IRInstruction::MOD, Mod);
	A(IRInstruction::READ_REG, ReadReg);
	A(IRInstruction::DISPATCH, Dispatch);
	A(IRInstruction::RET, Ret);
	A(IRInstruction::ADD, AddSub);
	A(IRInstruction::SUB, AddSub);
	A(IRInstruction::WRITE_REG, WriteReg);
	A(IRInstruction::JMP, Jmp);
	A(IRInstruction::BRANCH, Branch);
	A(IRInstruction::CALL, Call);
	A(IRInstruction::LDPC, LoadPC);
	A(IRInstruction::INCPC, IncPC);
	A(IRInstruction::WRITE_DEVICE, WriteDevice);
	A(IRInstruction::READ_DEVICE, ReadDevice);
	A(IRInstruction::PROBE_DEVICE, ProbeDevice);
	A(IRInstruction::CLZ, Clz);
	A(IRInstruction::BSWAP, BSwap);
	A(IRInstruction::AND, Bitwise);
	A(IRInstruction::OR, Bitwise);
	A(IRInstruction::XOR, Bitwise);
	A(IRInstruction::TRUNC, Trunc);
	A(IRInstruction::SX, Extend);
	A(IRInstruction::ZX, Extend);
	A(IRInstruction::CMPEQ, Compare);
	A(IRInstruction::CMPGT, Compare);
	A(IRInstruction::CMPGTE, Compare);
	A(IRInstruction::CMPLT, Compare);
	A(IRInstruction::CMPLTE, Compare);
	A(IRInstruction::CMPNE, Compare);
	A(IRInstruction::SHL, Shift);
	A(IRInstruction::SHR, Shift);
	A(IRInstruction::SAR, Shift);
	A(IRInstruction::ROR, Shift);
	A(IRInstruction::FLUSH_DTLB, FlushTlb);
	A(IRInstruction::FLUSH_ITLB, FlushTlb);
	A(IRInstruction::FLUSH_DTLB_ENTRY, FlushTlbEntry);
	A(IRInstruction::FLUSH_ITLB_ENTRY, FlushTlbEntry);
	A(IRInstruction::ADC_WITH_FLAGS, AdcFlags);
	A(IRInstruction::SBC_WITH_FLAGS, SbcFlags);
	A(IRInstruction::SET_ZN_FLAGS, ZNFlags);
	A(IRInstruction::TAKE_EXCEPTION, TakeException);
	A(IRInstruction::VERIFY, Verify);
	A(IRInstruction::COUNT, Count);

	A(IRInstruction::CMPSGT, CompareSigned);
	A(IRInstruction::CMPSGTE, CompareSigned);
	A(IRInstruction::CMPSLT, CompareSigned);
	A(IRInstruction::CMPSLTE, CompareSigned);

	A(IRInstruction::FMUL, FMul);
	A(IRInstruction::FDIV, FDiv);
	A(IRInstruction::FADD, FAdd);
	A(IRInstruction::FSUB, FSub);
	A(IRInstruction::FSQRT, FSqrt);

	A(IRInstruction::FCMP_LT, FCmp);
	A(IRInstruction::FCMP_LTE, FCmp);
	A(IRInstruction::FCMP_GT, FCmp);
	A(IRInstruction::FCMP_GTE, FCmp);
	A(IRInstruction::FCMP_EQ, FCmp);
	A(IRInstruction::FCMP_NE, FCmp);

	A(IRInstruction::FCVT_D_TO_S, FCvt_D_To_S);
	A(IRInstruction::FCVT_S_TO_D, FCvt_S_To_D);

	A(IRInstruction::FCVT_SI_TO_F, FCvt_SI_To_F);
	A(IRInstruction::FCVT_F_TO_SI, FCvt_F_To_SI);
	A(IRInstruction::FCVTT_F_TO_SI, FCvt_F_To_SI);
	A(IRInstruction::FCVT_UI_TO_F, FCvt_UI_To_F);
	A(IRInstruction::FCVT_F_TO_UI, FCvt_F_To_UI);
	A(IRInstruction::FCVTT_F_TO_UI, FCvt_F_To_UI);

	A(IRInstruction::FCTRL_GET_ROUND, FCtrl_GetRound);
	A(IRInstruction::FCTRL_SET_ROUND, FCtrl_SetRound);
	A(IRInstruction::FCTRL_GET_FLUSH_DENORMAL, FCtrl_GetFlush);
	A(IRInstruction::FCTRL_SET_FLUSH_DENORMAL, FCtrl_SetFlush);

	A(IRInstruction::VADDF, VAddF);
	A(IRInstruction::VADDI, VAddI);
	A(IRInstruction::VSUBF, VSubF);
	A(IRInstruction::VSUBI, VSubI);
	A(IRInstruction::VMULF, VMulF);
	A(IRInstruction::VMULI, VMulI);

	if(archsim::options::SystemMemoryModel == "cache") {
		A(IRInstruction::READ_MEM, ReadMemCache);
		A(IRInstruction::WRITE_MEM, WriteMemCache);
	} else if(archsim::options::SystemMemoryModel == "user") {
		A(IRInstruction::READ_MEM, ReadMemUser);
		A(IRInstruction::WRITE_MEM, WriteMemUser);
	} else {
		A(IRInstruction::READ_MEM, ReadMemGeneric);
		A(IRInstruction::WRITE_MEM, WriteMemGeneric);
	}
#undef A

	PrepareLowerers(ctx);

	return true;
}

bool X86LoweringContext::ABICalleeSave(const X86Register &reg)
{
	if(reg.hireg) {
		switch(reg.raw_index) {
			case 0:
				return false;// r8
			case 1:
				return false;// r9
			case 2:
				return false;// r10
			case 3:
				return false;// r11
			case 4:
				return true;// r12
			case 5:
				return true;// r13
			case 6:
				return true;// r14
			case 7:
				return true;// r15
			default:
				throw std::logic_error("");
		}
	} else {
		switch(reg.raw_index) {
			case 0:
				return false; // ax
			case 1:
				return false;// cx
			case 2:
				return false;// dx
			case 3:
				return true;// bx
			case 4:
				return true;// sp
			case 5:
				return true;// bp
			case 6:
				return false;// si
			case 7:
				return false;// di
			default:
				throw std::logic_error("");
		}
	}
}

std::vector<const X86Register*> X86LoweringContext::GetSavedRegisters()
{
	std::vector<const X86Register*> regs;

	for(int i = used_phys_regs.size()-1; i >= 0 ; --i) {
		if(used_phys_regs.get(i)) {
			auto &reg = get_allocable_register(i, 8);
			if(ABICalleeSave(reg)) {
				regs.push_back(&reg);
			}
		}
	}

	// only actually need to pop return reg if used
	if(ABICalleeSave(BLKJIT_RETURN(8))) {
		regs.push_back(&BLKJIT_RETURN(8));
	}
	if(ABICalleeSave(BLKJIT_REGSTATE_REG)) {
		regs.push_back(&BLKJIT_REGSTATE_REG);
	}
	if(ABICalleeSave(BLKJIT_CPUSTATE_REG)) {
		regs.push_back(&BLKJIT_CPUSTATE_REG);
	}

	return regs;
}

void X86LoweringContext::EmitPrologue()
{
	uint32_t max_stack = GetStackFrameSize();

	auto saved_regs = GetSavedRegisters();
	for(auto i : saved_regs) {
		GetEncoder().push(*i);
	}

	if(max_stack & 15) {
		max_stack = (max_stack & ~15) + 16;
	}
	if((saved_regs.size() & 0x1)) {
		max_stack += 8;
	}
	if(max_stack) GetEncoder().sub(max_stack, REG_RSP);
}


void X86LoweringContext::EmitEpilogue()
{
	auto saved_regs = GetSavedRegisters();

	uint32_t max_stack = GetStackFrameSize();
	if(max_stack & 15) {
		max_stack = (max_stack & ~15) + 16;
	}
	if((saved_regs.size() & 0x1)) {
		max_stack += 8;
	}
	if(max_stack)
		GetEncoder().add(max_stack, REG_RSP);

	// need to pop used regs
	for(int i = saved_regs.size()-1; i >= 0 ; --i) {
		GetEncoder().pop(*saved_regs.at(i));
	}
}


bool X86LoweringContext::LowerHeader(const TranslationContext &ctx)
{
	EmitPrologue();

	GetEncoder().mov(REG_RDI, BLKJIT_REGSTATE_REG);
	GetEncoder().mov(REG_RSI, BLKJIT_CPUSTATE_REG);

	return true;
}

bool X86LoweringContext::PerformRelocations(const TranslationContext &ctx)
{
	for(auto reloc : GetBlockRelocations()) {
		int32_t value = GetBlockOffset(reloc.second);
		value -= reloc.first;
		value -= 4;

		uint32_t *slot = (uint32_t *)&GetEncoder().get_buffer()[reloc.first];
		*slot = value;
	}

	return true;
}

MCLoweringContext::offset_t X86LoweringContext::GetEncoderOffset()
{
	return GetEncoder().current_offset();
}

void X86LoweringContext::emit_save_reg_state(int num_operands, stack_map_t &stack_map, bool &fix_stack, uint32_t live_regs)
{
	uint32_t stack_offset = 0;
	// XXX host architecture dependent
	// On X86-64, the stack pointer on entry to a function must have an offset
	// of 0x8 (such that %rsp + 8 is 16 byte aligned). Since the call instruction
	// will push 8 bytes onto the stack, plus we save 2 more regs (CPUState, Regstate),
	fix_stack = true;

	stack_map.clear();

	//First, count required stack slots
	for(int i = used_phys_regs.size()-1; i >= 0; i--) {
		if(used_phys_regs.get(i) && (live_regs & (1 << i))) {
			stack_offset += 8;
			fix_stack = !fix_stack;
		}
	}

	if(fix_stack) {
		GetEncoder().sub(8, REG_RSP);
//		GetEncoder().push(REG_RAX);
	}

	for(int i = used_phys_regs.size()-1; i >= 0; i--) {
		if(used_phys_regs.get(i) && (live_regs & (1 << i))) {
			auto& reg = get_allocable_register(i, 8);

			stack_offset -= 8;
			stack_map[&reg] = stack_offset;
			GetEncoder().push(reg);

		}
	}
}

void X86LoweringContext::emit_restore_reg_state(bool fix_stack, uint32_t live_regs)
{

	for(unsigned int i = 0; i < used_phys_regs.size(); ++i) {
		if(used_phys_regs.get(i) && (live_regs & (1 << i))) {
			GetEncoder().pop(get_allocable_register(i, 8));
		}
	}
	if(fix_stack) {
		GetEncoder().add(8, REG_RSP);
//		GetEncoder().pop(REG_RAX);
	}
}

void X86LoweringContext::encode_operand_function_argument(const IROperand *oper, const X86Register& target_reg, stack_map_t &stack_map)
{
	uint8_t stack_offset_adjust = 8;
	if(stack_map.size() % 2) {
		stack_offset_adjust = 0;
	}

	if (oper->is_constant() == IROperand::CONSTANT) {
		if(oper->value == 0) GetEncoder().xorr(target_reg, target_reg);
		else GetEncoder().mov(oper->value, target_reg);
	} else if (oper->is_vreg()) {
//		if(get_allocable_register(oper->alloc_data, 8) == REG_RBX) {
//			GetEncoder().mov(REGS_RBX(oper->size), target_reg);
//		} else
		if (oper->is_alloc_reg()) {
			auto &reg = get_allocable_register(oper->alloc_data, 8);
			GetEncoder().mov(X86Memory::get(REG_RSP, stack_map.at(&reg)), target_reg);
		} else {
			int64_t stack_offset = (stack_offset_adjust + oper->alloc_data + (8*stack_map.size()));
			GetEncoder().lea(X86Memory::get(REG_RSP, stack_offset), target_reg);
		}
	} else {
		assert(false);
	}
}

void X86LoweringContext::encode_operand_to_reg(const shared::IROperand *operand, const X86Register& reg)
{
	switch (operand->type) {
		case IROperand::CONSTANT: {
			if (operand->value == 0) {
				GetEncoder().xorr(reg, reg);
			} else {
				GetEncoder().mov(operand->value, reg);
			}

			break;
		}

		case IROperand::VREG: {
			switch (operand->alloc_mode) {
				case IROperand::ALLOCATED_STACK:
					GetEncoder().mov(stack_from_operand(operand), reg);
					break;

				case IROperand::ALLOCATED_REG:
					GetEncoder().mov(register_from_operand(operand, reg.size), reg);
					break;

				default:
					assert(false);
			}

			break;
		}

		default:
			assert(false);
	}
}
