/*
 * X86LoweringContext.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/InstructionLowerer.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86Lowerers.h"
#include "blockjit/blockjit-abi.h"

#include "util/LogContext.h"

UseLogContext(LogLower);

using namespace captive::arch::jit::lowering;
using namespace captive::arch::jit::lowering::x86;
using namespace captive::arch::x86;

X86LoweringContext::X86LoweringContext(uint32_t stack_frame_size, encoder_t &encoder) : LoweringContext(stack_frame_size), _encoder(encoder), _stack_fixed(false)
{

}

X86LoweringContext::~X86LoweringContext()
{

}


#define LowerType(X) Lower ## X Singleton_Lower ## X;
#define LowerTypeTS(X) Lower ## X Singleton_Lower ## X;
#include "blockjit/block-compiler/lowering/x86/LowerTypes.h"
#undef LowerType

NOPInstructionLowerer Singleton_LowerNop;

bool X86LoweringContext::Prepare(const TranslationContext &ctx, BlockCompiler &compiler)
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
	A(IRInstruction::MUL, Mul);
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

	if(archsim::options::SystemMemoryModel == "function") {
		A(IRInstruction::READ_MEM, ReadMemFunction);
		A(IRInstruction::READ_MEM_USER, ReadUserMemFunction);
		A(IRInstruction::WRITE_MEM, WriteMemFunction);
		A(IRInstruction::WRITE_MEM_USER, WriteUserMemFunction);
	} else if(archsim::options::SystemMemoryModel == "cache") {
		A(IRInstruction::READ_MEM, ReadMemCache);
		A(IRInstruction::READ_MEM_USER, ReadUserMemCache);
		A(IRInstruction::WRITE_MEM, WriteMemCache);
		A(IRInstruction::WRITE_MEM_USER, WriteUserMemCache);
	} else if(archsim::options::SystemMemoryModel == "user") {
		A(IRInstruction::READ_MEM, ReadMemUser);
		A(IRInstruction::READ_MEM_USER, ReadUserMemGeneric);
		A(IRInstruction::WRITE_MEM, WriteMemUser);
		A(IRInstruction::WRITE_MEM_USER, WriteUserMemGeneric);
	} else {
		A(IRInstruction::READ_MEM, ReadMemGeneric);
		A(IRInstruction::READ_MEM_USER, ReadUserMemGeneric);
		A(IRInstruction::WRITE_MEM, WriteMemGeneric);
		A(IRInstruction::WRITE_MEM_USER, WriteUserMemGeneric);
	}
#undef A

	PrepareLowerers(ctx, compiler);

	return true;
}

bool X86LoweringContext::LowerHeader(const TranslationContext &ctx)
{
	uint32_t max_stack = GetStackFrameSize();
	if(max_stack & 15) {
		max_stack = (max_stack & ~15) + 16;
	}

	if(max_stack) GetEncoder().sub(max_stack, REG_RSP);

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

LoweringContext::offset_t X86LoweringContext::GetEncoderOffset()
{
	return GetEncoder().current_offset();
}

