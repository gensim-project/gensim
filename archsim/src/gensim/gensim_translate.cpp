/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_translate.h"
#include "translate/llvm/LLVMTranslationContext.h"

#include <llvm/IR/Constants.h>

using namespace gensim;

llvm::Value *BaseLLVMTranslate::EmitRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size, int offset)
{
	llvm::Value *ptr = GetRegisterPtr(ctx, size, offset);
	auto value = ctx.Builder.CreateLoad(ptr);
	return value;
}

bool BaseLLVMTranslate::EmitRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset, llvm::Value *value)
{
	llvm::Value *ptr = GetRegisterPtr(ctx, size_in_bytes, offset);
	value = ctx.Builder.CreateBitCast(value, llvm::IntegerType::getIntNTy(ctx.LLVMCtx, size_in_bytes*8));
	ctx.Builder.CreateStore(value, ptr);
	return true;
}

llvm::Value* BaseLLVMTranslate::EmitMemoryRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value* address)
{
	llvm::Function *fn_ptr = nullptr;
	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			fn_ptr = ctx.Functions.blkRead8;
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead8;
			break;
		case 2:
			fn_ptr = ctx.Functions.blkRead16;
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead16;
			break;
		case 4:
			fn_ptr = ctx.Functions.blkRead32;
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead32;
			break;
		case 8:
			fn_ptr = ctx.Functions.blkRead64;
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead64;
			break;
		default:
			UNIMPLEMENTED;
	}

	auto value = ctx.Builder.CreateCall(fn_ptr, {ctx.Values.state_block_ptr, address, llvm::ConstantInt::get(ctx.Types.i32, interface)});

	if(archsim::options::Trace) {
		ctx.Builder.CreateCall(trace_fn_ptr, {ctx.GetThreadPtr(), address, value});
	}

	return value;
}

void BaseLLVMTranslate::EmitMemoryWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value* address, llvm::Value* value)
{
	llvm::Function *fn_ptr = nullptr;
	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			fn_ptr = ctx.Functions.cpuWrite8;
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite8;
			break;
		case 2:
			fn_ptr = ctx.Functions.cpuWrite16;
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite16;
			break;
		case 4:
			fn_ptr = ctx.Functions.cpuWrite32;
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite32;
			break;
		case 8:
			fn_ptr = ctx.Functions.cpuWrite64;
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite64;
			break;
		default:
			UNIMPLEMENTED;
	}

	if(archsim::options::Trace) {
		ctx.Builder.CreateCall(trace_fn_ptr, {ctx.GetThreadPtr(), address, value});
	}
	ctx.Builder.CreateCall(fn_ptr, {GetThreadPtr(ctx), llvm::ConstantInt::get(ctx.Types.i32, interface), address, value});
}

void BaseLLVMTranslate::EmitTakeException(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, llvm::Value* category, llvm::Value* data)
{
	ctx.Builder.CreateCall(ctx.Functions.TakeException, {GetThreadPtr(ctx), category, data});
}

llvm::Value* BaseLLVMTranslate::GetRegisterPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset)
{
	llvm::Value *ptr = GetRegfilePtr(ctx);
	ptr = ctx.Builder.CreatePtrToInt(ptr, ctx.Types.i64);
	ptr = ctx.Builder.CreateAdd(ptr, llvm::ConstantInt::get(ctx.Types.i64, offset));
	ptr = ctx.Builder.CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8));
	return ptr;
}

llvm::Value* BaseLLVMTranslate::GetRegfilePtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx)
{
	return ctx.Values.reg_file_ptr;
}

llvm::Value* BaseLLVMTranslate::GetThreadPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx)
{
	return ctx.GetThreadPtr();
}

void BaseLLVMTranslate::EmitAdcWithFlags(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int bits, llvm::Value* lhs, llvm::Value* rhs, llvm::Value* carry)
{
	llvm::IntegerType *base_itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits);
	llvm::IntegerType *itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits*2);

	lhs = ctx.Builder.CreateZExtOrTrunc(lhs, base_itype);
	rhs = ctx.Builder.CreateZExtOrTrunc(rhs, base_itype);
	carry = ctx.Builder.CreateZExtOrTrunc(carry, base_itype);

	// cast each value to correct bitwidth
	llvm::Value *extended_lhs = ctx.Builder.CreateZExtOrTrunc(lhs, itype);
	llvm::Value *extended_rhs = ctx.Builder.CreateZExtOrTrunc(rhs, itype);
	llvm::Value *extended_carry = ctx.Builder.CreateZExtOrTrunc(carry, itype);

	// produce result
	llvm::Value *extended_result = ctx.Builder.CreateAdd(extended_rhs, extended_carry);
	extended_result = ctx.Builder.CreateAdd(extended_lhs, extended_result);

	llvm::Value *base_result = ctx.Builder.CreateTrunc(extended_result, base_itype);

	// calculate flags
	llvm::Value *Z = ctx.Builder.CreateICmpEQ(base_result, llvm::ConstantInt::get(base_itype, 0));
	Z = ctx.Builder.CreateZExt(Z, ctx.Types.i8);

	llvm::Value *N = ctx.Builder.CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));
	N = ctx.Builder.CreateZExt(N, ctx.Types.i8);

	llvm::Value *C = ctx.Builder.CreateICmpUGT(extended_result, ctx.Builder.CreateZExt(llvm::ConstantInt::get(base_itype, -1ull), itype));
	C = ctx.Builder.CreateZExt(C, ctx.Types.i8);

	llvm::Value *lhs_sign = ctx.Builder.CreateICmpSLT(lhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *rhs_sign = ctx.Builder.CreateICmpSLT(rhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *result_sign = ctx.Builder.CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));

	llvm::Value *V_lhs = ctx.Builder.CreateICmpEQ(lhs_sign, rhs_sign);
	llvm::Value *V_rhs = ctx.Builder.CreateICmpNE(lhs_sign, result_sign);

	llvm::Value *V = ctx.Builder.CreateAnd(V_lhs, V_rhs);
	V = ctx.Builder.CreateZExt(V, ctx.Types.i8);

	auto C_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("C");
	auto V_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("V");
	auto N_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("N");
	auto Z_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("Z");

	EmitRegisterWrite(ctx, 1, C_desc.GetOffset(), C);
	EmitRegisterWrite(ctx, 1, V_desc.GetOffset(), V);
	EmitRegisterWrite(ctx, 1, N_desc.GetOffset(), N);
	EmitRegisterWrite(ctx, 1, Z_desc.GetOffset(), Z);
}

void BaseLLVMTranslate::EmitSbcWithFlags(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int bits, llvm::Value* lhs, llvm::Value* rhs, llvm::Value* carry)
{
	llvm::IntegerType *base_itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits);
	llvm::IntegerType *itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits*2);

	lhs = ctx.Builder.CreateZExtOrTrunc(lhs, base_itype);
	rhs = ctx.Builder.CreateZExtOrTrunc(rhs, base_itype);
	carry = ctx.Builder.CreateZExtOrTrunc(carry, base_itype);

	// cast each value to correct bitwidth
	llvm::Value *extended_lhs = ctx.Builder.CreateZExtOrTrunc(lhs, itype);
	llvm::Value *extended_rhs = ctx.Builder.CreateZExtOrTrunc(rhs, itype);
	llvm::Value *extended_carry = ctx.Builder.CreateZExtOrTrunc(carry, itype);

	// produce result
	llvm::Value *result = ctx.Builder.CreateAdd(extended_rhs, extended_carry);
	result = ctx.Builder.CreateSub(extended_lhs, result);

	llvm::Value *base_result = ctx.Builder.CreateTrunc(result, base_itype);

	// calculate flags
	llvm::Value *Z = ctx.Builder.CreateICmpEQ(base_result, llvm::ConstantInt::get(base_itype, 0));
	Z = ctx.Builder.CreateZExt(Z, ctx.Types.i8);

	llvm::Value *N = ctx.Builder.CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));
	N = ctx.Builder.CreateZExt(N, ctx.Types.i8);

	llvm::Value *C = ctx.Builder.CreateICmpUGT(result, ctx.Builder.CreateZExt(llvm::ConstantInt::get(base_itype, -1ull), itype));
	C = ctx.Builder.CreateZExt(C, ctx.Types.i8);

	llvm::Value *lhs_sign = ctx.Builder.CreateICmpSLT(lhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *rhs_sign = ctx.Builder.CreateICmpSLT(rhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *result_sign = ctx.Builder.CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));

	llvm::Value *V_lhs = ctx.Builder.CreateICmpNE(lhs_sign, rhs_sign);
	llvm::Value *V_rhs = ctx.Builder.CreateICmpEQ(rhs_sign, result_sign);

	llvm::Value *V = ctx.Builder.CreateAnd(V_lhs, V_rhs);
	V = ctx.Builder.CreateZExt(V, ctx.Types.i8);

	auto C_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("C");
	auto V_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("V");
	auto N_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("N");
	auto Z_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("Z");

	EmitRegisterWrite(ctx, 1, C_desc.GetOffset(), C);
	EmitRegisterWrite(ctx, 1, V_desc.GetOffset(), V);
	EmitRegisterWrite(ctx, 1, N_desc.GetOffset(), N);
	EmitRegisterWrite(ctx, 1, Z_desc.GetOffset(), Z);
}

void BaseLLVMTranslate::QueueDynamicBlock(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, std::map<uint16_t, llvm::BasicBlock*>& dynamic_blocks, std::list<uint16_t>& dynamic_block_queue, uint16_t queued_block)
{
	if(dynamic_blocks.count(queued_block)) {
		return;
	}
	dynamic_blocks[queued_block] = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.Builder.GetInsertBlock()->getParent());
	dynamic_block_queue.push_back(queued_block);
}

void BaseLLVMTranslate::EmitTraceBankedRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* regnum, int size, llvm::Value* value_ptr)
{
	if(archsim::options::Trace) {
		ctx.Builder.CreateCall(ctx.Functions.cpuTraceBankedRegisterWrite, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.Builder.CreateZExtOrTrunc(regnum, ctx.Types.i32), llvm::ConstantInt::get(ctx.Types.i32, size), value_ptr});
	}
}

void BaseLLVMTranslate::EmitTraceRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* value)
{
	if(archsim::options::Trace) {
		ctx.Builder.CreateCall(ctx.Functions.cpuTraceRegisterWrite, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.Builder.CreateZExtOrTrunc(value, ctx.Types.i64)});
	}
}

void BaseLLVMTranslate::EmitTraceBankedRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* regnum, int size, llvm::Value* value_ptr)
{
	if(archsim::options::Trace) {
		ctx.Builder.CreateCall(ctx.Functions.cpuTraceBankedRegisterRead, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.Builder.CreateZExtOrTrunc(regnum, ctx.Types.i32), llvm::ConstantInt::get(ctx.Types.i32, size), value_ptr});
	}
}

void BaseLLVMTranslate::EmitTraceRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* value)
{
	if(archsim::options::Trace) {
		ctx.Builder.CreateCall(ctx.Functions.cpuTraceRegisterRead, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.Builder.CreateZExtOrTrunc(value, ctx.Types.i64)});
	}
}
