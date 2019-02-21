/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_translate.h"

#if ARCHSIM_ENABLE_LLVM
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"

#include <llvm/IR/Constants.h>

using namespace gensim;

llvm::Value* BaseLLVMTranslate::EmitRegisterRead(Builder& builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, const archsim::RegisterFileEntryDescriptor &entry, llvm::Value *index)
{
	return ctx.LoadGuestRegister(builder, entry, index);
}
bool BaseLLVMTranslate::EmitRegisterWrite(Builder& builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, const archsim::RegisterFileEntryDescriptor &entry, llvm::Value *index, llvm::Value *value)
{
	ctx.StoreGuestRegister(builder, entry, index, value);
	return true;
}

#define FAST_READS
//#define FASTER_READS
//#define FASTER_WRITES
#define FAST_WRITES

llvm::Value* BaseLLVMTranslate::EmitMemoryRead(llvm::IRBuilder<> &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value* address)
{
	return ctx.GetMemoryEmitter().EmitMemoryRead(builder, address, size_in_bytes, interface, 0);
}

void BaseLLVMTranslate::EmitMemoryWrite(llvm::IRBuilder<> &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value* address, llvm::Value* value)
{
	ctx.GetMemoryEmitter().EmitMemoryWrite(builder, address, value, size_in_bytes, interface, 0);
}

void BaseLLVMTranslate::EmitTakeException(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, llvm::Value* category, llvm::Value* data)
{
	builder.CreateCall(ctx.Functions.TakeException, {ctx.GetThreadPtr(builder), category, data});
}

llvm::Value* BaseLLVMTranslate::GetRegisterPtr(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset)
{
	llvm::Value *ptr = GetRegfilePtr(ctx);
	ptr = builder.CreatePtrToInt(ptr, ctx.Types.i64);
	ptr = builder.CreateAdd(ptr, llvm::ConstantInt::get(ctx.Types.i64, offset));
	ptr = builder.CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8));
	return ptr;
}

llvm::Value* BaseLLVMTranslate::GetRegfilePtr(archsim::translate::translate_llvm::LLVMTranslationContext& ctx)
{
	return ctx.Values.reg_file_ptr;
}

llvm::Value* BaseLLVMTranslate::GetThreadPtr(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx)
{
	return ctx.GetThreadPtr(builder);
}

void BaseLLVMTranslate::EmitAdcWithFlags(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int bits, llvm::Value* lhs, llvm::Value* rhs, llvm::Value* carry)
{
	// TODO: this leads to not very efficient code. It would be better to
	// figure out a better way of handling this, possibly by modifying LLVM
	// (new intrinsic or custom lowering)

	// carry must be 0 or 1
	llvm::Value *carry_expr = builder.CreateOr(builder.CreateICmpEQ(carry, llvm::ConstantInt::get(carry->getType(), 0)), builder.CreateICmpEQ(carry, llvm::ConstantInt::get(carry->getType(), 1)));
	builder.CreateCall(ctx.Functions.assume, { carry_expr });

	llvm::IntegerType *base_itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits);
	llvm::IntegerType *itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits*2);

	lhs = builder.CreateZExtOrTrunc(lhs, base_itype);
	rhs = builder.CreateZExtOrTrunc(rhs, base_itype);
	carry = builder.CreateZExtOrTrunc(carry, base_itype);

	// cast each value to correct bitwidth
	llvm::Value *extended_lhs = builder.CreateZExtOrTrunc(lhs, itype);
	llvm::Value *extended_rhs = builder.CreateZExtOrTrunc(rhs, itype);
	llvm::Value *extended_carry = builder.CreateZExtOrTrunc(carry, itype);

	// produce result
	llvm::Value *extended_result = builder.CreateAdd(extended_rhs, extended_carry);

	llvm::Value *callee;
	switch(bits) {
		case 8:
			callee = ctx.Functions.uadd_with_overflow_8;
			break;
		case 16:
			callee = ctx.Functions.uadd_with_overflow_16;
			break;
		case 32:
			callee = ctx.Functions.uadd_with_overflow_32;
			break;
		case 64:
			callee = ctx.Functions.uadd_with_overflow_64;
			break;
		default:
			UNIMPLEMENTED;
	}
	llvm::Value *partial_result = builder.CreateCall(callee, {lhs, rhs});
	llvm::Value *full_result = builder.CreateCall(callee, {builder.CreateExtractValue(partial_result, {0}), carry});

	llvm::Value *base_result = builder.CreateExtractValue(full_result, {0});

	// calculate flags
	llvm::Value *Z = builder.CreateICmpEQ(base_result, llvm::ConstantInt::get(base_itype, 0));
	Z = builder.CreateZExt(Z, ctx.Types.i8);

	llvm::Value *N = builder.CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));
	N = builder.CreateZExt(N, ctx.Types.i8);

	llvm::Value *C = builder.CreateOr(builder.CreateExtractValue(partial_result, {1}), builder.CreateExtractValue(full_result, {1}));
	C = builder.CreateZExt(C, ctx.Types.i8);

	switch(bits) {
		case 8:
			callee = ctx.Functions.sadd_with_overflow_8;
			break;
		case 16:
			callee = ctx.Functions.sadd_with_overflow_16;
			break;
		case 32:
			callee = ctx.Functions.sadd_with_overflow_32;
			break;
		case 64:
			callee = ctx.Functions.sadd_with_overflow_64;
			break;
		default:
			UNIMPLEMENTED;
	}

	partial_result = builder.CreateCall(callee, {lhs, rhs});
	full_result = builder.CreateCall(callee, {builder.CreateExtractValue(partial_result, {0}), carry});
	llvm::Value *V = builder.CreateOr(builder.CreateExtractValue(partial_result, {1}), builder.CreateExtractValue(full_result, {1}));
	V = builder.CreateZExt(V, ctx.Types.i8);

	auto &c_reg = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("C");
	auto &v_reg = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("V");
	auto &n_reg = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("N");
	auto &z_reg = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("Z");

	EmitRegisterWrite(builder, ctx, c_reg, nullptr, C);
	EmitRegisterWrite(builder, ctx, v_reg, nullptr, V);
	EmitRegisterWrite(builder, ctx, n_reg, nullptr, N);
	EmitRegisterWrite(builder, ctx, z_reg, nullptr, Z);

	EmitTraceRegisterWrite(builder, ctx, c_reg.GetID(), C);
	EmitTraceRegisterWrite(builder, ctx, v_reg.GetID(), V);
	EmitTraceRegisterWrite(builder, ctx, n_reg.GetID(), N);
	EmitTraceRegisterWrite(builder, ctx, z_reg.GetID(), Z);
}

void BaseLLVMTranslate::EmitSbcWithFlags(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int bits, llvm::Value* lhs, llvm::Value* rhs, llvm::Value* carry)
{
	// TODO: this leads to not very efficient code. It would be better to
	// figure out a better way of handling this, possibly by modifying LLVM
	// (new intrinsic or custom lowering)

	// carry must be 0 or 1
	llvm::Value *carry_expr = builder.CreateOr(builder.CreateICmpEQ(carry, llvm::ConstantInt::get(carry->getType(), 0)), builder.CreateICmpEQ(carry, llvm::ConstantInt::get(carry->getType(), 1)));
	builder.CreateCall(ctx.Functions.assume, { carry_expr });

	llvm::IntegerType *base_itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits);
	llvm::IntegerType *itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits*2);

	lhs = builder.CreateZExtOrTrunc(lhs, base_itype);
	rhs = builder.CreateZExtOrTrunc(rhs, base_itype);
	carry = builder.CreateZExtOrTrunc(carry, base_itype);

	// cast each value to correct bitwidth
	llvm::Value *extended_lhs = builder.CreateZExtOrTrunc(lhs, itype);
	llvm::Value *extended_rhs = builder.CreateZExtOrTrunc(rhs, itype);
	llvm::Value *extended_carry = builder.CreateZExtOrTrunc(carry, itype);

	llvm::Value *callee;
	switch(bits) {
		case 8:
			callee = ctx.Functions.usub_with_overflow_8;
			break;
		case 16:
			callee = ctx.Functions.usub_with_overflow_16;
			break;
		case 32:
			callee = ctx.Functions.usub_with_overflow_32;
			break;
		case 64:
			callee = ctx.Functions.usub_with_overflow_64;
			break;
		default:
			UNIMPLEMENTED;
	}

	// produce result for carry
	llvm::Value *partial_result = builder.CreateCall(callee, {lhs, rhs});
	llvm::Value *full_result = builder.CreateCall(callee, {builder.CreateExtractValue(partial_result, {0}), carry});
	llvm::Value *result = builder.CreateExtractValue(full_result, {0});


	// calculate flags
	llvm::Value *Z = builder.CreateICmpEQ(result, llvm::ConstantInt::get(base_itype, 0));
	Z = builder.CreateZExt(Z, ctx.Types.i8);

	llvm::Value *N = builder.CreateICmpSLT(result, llvm::ConstantInt::get(base_itype, 0));
	N = builder.CreateZExt(N, ctx.Types.i8);

	llvm::Value *C = builder.CreateOr(builder.CreateExtractValue(partial_result, {1}), builder.CreateExtractValue(full_result, {1}));
	C = builder.CreateZExt(C, ctx.Types.i8);

	switch(bits) {
		case 8:
			callee = ctx.Functions.ssub_with_overflow_8;
			break;
		case 16:
			callee = ctx.Functions.ssub_with_overflow_16;
			break;
		case 32:
			callee = ctx.Functions.ssub_with_overflow_32;
			break;
		case 64:
			callee = ctx.Functions.ssub_with_overflow_64;
			break;
		default:
			UNIMPLEMENTED;
	}

	partial_result = builder.CreateCall(callee, {lhs, rhs});
	full_result = builder.CreateCall(callee, {builder.CreateExtractValue(partial_result, {0}), carry});
	llvm::Value *V = builder.CreateOr(builder.CreateExtractValue(partial_result, {1}), builder.CreateExtractValue(full_result, {1}));
	V = builder.CreateZExt(V, ctx.Types.i8);

	auto &C_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("C");
	auto &V_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("V");
	auto &N_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("N");
	auto &Z_desc = ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("Z");

	EmitRegisterWrite(builder, ctx, C_desc, nullptr, C);
	EmitRegisterWrite(builder, ctx, V_desc, nullptr, V);
	EmitRegisterWrite(builder, ctx, N_desc, nullptr, N);
	EmitRegisterWrite(builder, ctx, Z_desc, nullptr, Z);

	EmitTraceRegisterWrite(builder, ctx, C_desc.GetID(), C);
	EmitTraceRegisterWrite(builder, ctx, V_desc.GetID(), V);
	EmitTraceRegisterWrite(builder, ctx, N_desc.GetID(), N);
	EmitTraceRegisterWrite(builder, ctx, Z_desc.GetID(), Z);
}

void BaseLLVMTranslate::QueueDynamicBlock(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, std::map<uint16_t, llvm::BasicBlock*>& dynamic_blocks, std::list<uint16_t>& dynamic_block_queue, uint16_t queued_block)
{
	if(dynamic_blocks.count(queued_block)) {
		return;
	}
	dynamic_blocks[queued_block] = llvm::BasicBlock::Create(ctx.LLVMCtx, "", builder.GetInsertBlock()->getParent());
	dynamic_block_queue.push_back(queued_block);
}

void BaseLLVMTranslate::EmitTraceBankedRegisterWrite(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* regnum, int size, llvm::Value* value)
{
	if(archsim::options::Trace) {
		llvm::Value *value_ptr = ctx.GetTraceStackSlot(value->getType());
		builder.CreateStore(value, value_ptr);

		value_ptr = builder.CreatePointerCast(value_ptr, ctx.Types.i8Ptr);
		builder.CreateCall(ctx.Functions.cpuTraceBankedRegisterWrite, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i32, id), builder.CreateZExtOrTrunc(regnum, ctx.Types.i32), llvm::ConstantInt::get(ctx.Types.i32, size), value_ptr});
	}
}

void BaseLLVMTranslate::EmitTraceRegisterWrite(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* value)
{
	if(archsim::options::Trace) {
		builder.CreateCall(ctx.Functions.cpuTraceRegisterWrite, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i32, id), builder.CreateZExtOrTrunc(value, ctx.Types.i64)});
	}
}

void BaseLLVMTranslate::EmitTraceBankedRegisterRead(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* regnum, int size, llvm::Value* value)
{
	if(archsim::options::Trace) {
		llvm::Value *value_ptr = ctx.GetTraceStackSlot(value->getType());
		builder.CreateStore(value, value_ptr);

		value_ptr = builder.CreatePointerCast(value_ptr, ctx.Types.i8Ptr);
		builder.CreateCall(ctx.Functions.cpuTraceBankedRegisterRead, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i32, id), builder.CreateZExtOrTrunc(regnum, ctx.Types.i32), llvm::ConstantInt::get(ctx.Types.i32, size), value_ptr});
	}
}

void BaseLLVMTranslate::EmitTraceRegisterRead(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* value)
{
	if(archsim::options::Trace) {
		builder.CreateCall(ctx.Functions.cpuTraceRegisterRead, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i32, id), builder.CreateZExtOrTrunc(value, ctx.Types.i64)});
	}
}

void BaseLLVMTranslate::EmitIncrementCounter(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, archsim::util::Counter64& counter, uint32_t value)
{
	llvm::Value *ptr = llvm::ConstantInt::get(ctx.Types.i64, (uint64_t)counter.get_ptr());
	ptr = builder.CreateIntToPtr(ptr, ctx.Types.i64Ptr);
	llvm::Value *val = builder.CreateLoad(ptr);
	val = builder.CreateAdd(val, llvm::ConstantInt::get(ctx.Types.i64, value));
	builder.CreateStore(val, ptr);
}
void BaseLLVMTranslate::EmitIncrementHistogram(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, archsim::util::Histogram& histogram, uint64_t key, uint32_t value)
{
	llvm::Value *ptr = llvm::ConstantInt::get(ctx.Types.i64, (uint64_t)histogram.get_value_ptr_at_index(key));
	ptr = builder.CreateIntToPtr(ptr, ctx.Types.i64Ptr);
	llvm::Value *val = builder.CreateLoad(ptr);
	val = builder.CreateAdd(val, llvm::ConstantInt::get(ctx.Types.i64, value));
	builder.CreateStore(val, ptr);
}

#endif
