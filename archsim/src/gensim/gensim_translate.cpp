/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_translate.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"

#include <llvm/IR/Constants.h>

using namespace gensim;

llvm::Value *BaseLLVMTranslate::EmitRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset)
{
	llvm::Value *ptr = ctx.GetRegPtr(offset, llvm::IntegerType::getIntNTy(ctx.LLVMCtx, size_in_bytes*8));
	auto value = ctx.GetBuilder().CreateLoad(ptr);
	return value;
}

bool BaseLLVMTranslate::EmitRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset, llvm::Value *value)
{
	llvm::Value *ptr = ctx.GetRegPtr(offset, llvm::IntegerType::getIntNTy(ctx.LLVMCtx, size_in_bytes*8));
	value = ctx.GetBuilder().CreateBitCast(value, llvm::IntegerType::getIntNTy(ctx.LLVMCtx, size_in_bytes*8));
	ctx.GetBuilder().CreateStore(value, ptr);
	return true;
}

//#define FAST_READS
#define FASTER_READS
#define FASTER_WRITES
//#define FAST_WRITES

llvm::Value* BaseLLVMTranslate::EmitMemoryRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value* address)
{
#ifdef FASTER_READS
	llvm::Value *mem_offset = llvm::ConstantInt::get(ctx.Types.i64, 0x80000000);
	address = ctx.GetBuilder().CreateAdd(address, mem_offset);
	llvm::IntToPtrInst *ptr = (llvm::IntToPtrInst *)ctx.GetBuilder().CreateCast(llvm::Instruction::IntToPtr, address, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8, 0));
	if(ptr->getValueID() == llvm::Instruction::InstructionVal + llvm::Instruction::IntToPtr) {
		((llvm::IntToPtrInst*)ptr)->setMetadata("aaai", llvm::MDNode::get(ctx.LLVMCtx, { llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, archsim::translate::translate_llvm::TAG_MEM_ACCESS)) }));
	}
	auto value = ctx.GetBuilder().CreateLoad(ptr);

	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead8;
			break;
		case 2:
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead16;
			break;
		case 4:
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead32;
			break;
		case 8:
			trace_fn_ptr = ctx.Functions.cpuTraceMemRead64;
			break;
		default:
			UNIMPLEMENTED;
	}
#else

#ifdef FAST_READS
	llvm::Value *cache_ptr = ctx.GetBuilder().CreatePtrToInt(ctx.GetStateBlockPointer("memory_cache_" + std::to_string(interface)), ctx.Types.i64);
	llvm::Value *page_index = ctx.GetBuilder().CreateLShr(address, 12);

	if(archsim::options::Verbose) {
		EmitIncrementCounter(ctx, ctx.GetThread()->GetMetrics().Reads);
	}

	// TODO: get rid of these magic numbers (number of entries in cache, and cache entry size)
	llvm::Value *cache_entry = ctx.GetBuilder().CreateURem(page_index, llvm::ConstantInt::get(ctx.Types.i64, 1024));
	cache_entry = ctx.GetBuilder().CreateMul(cache_entry, llvm::ConstantInt::get(ctx.Types.i64, 16));
	cache_entry = ctx.GetBuilder().CreateAdd(cache_ptr, cache_entry);

	llvm::Value *tag = ctx.GetBuilder().CreateIntToPtr(cache_entry, ctx.Types.i64Ptr);
	tag = ctx.GetBuilder().CreateLoad(tag);

	// does tag match? make sure end of memory access falls on correct page
	llvm::Value *offset_page_base = ctx.GetBuilder().CreateAdd(address, llvm::ConstantInt::get(ctx.Types.i64, size_in_bytes-1));
	offset_page_base = ctx.GetBuilder().CreateAnd(offset_page_base, ~archsim::Address::PageMask);

	llvm::Value *tag_match = ctx.GetBuilder().CreateICmpEQ(tag, offset_page_base);
	llvm::BasicBlock *match_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	llvm::BasicBlock *nomatch_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	auto br = ctx.GetBuilder().CreateCondBr(tag_match, match_block, nomatch_block);
	auto mdnode = llvm::MDNode::get(ctx.LLVMCtx, {llvm::MDString::get(ctx.LLVMCtx, "branch_weights"), llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, 95)), llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, 5))});
	br->setMetadata(llvm::LLVMContext::MD_prof, mdnode);

	ctx.GetBuilder().SetInsertPoint(match_block);

	llvm::Value *page_offset = ctx.GetBuilder().CreateAnd(address, archsim::Address::PageMask);
	llvm::Value *ptr = ctx.GetBuilder().CreateAdd(cache_entry, llvm::ConstantInt::get(ctx.Types.i64, 8));
	ptr = ctx.GetBuilder().CreateIntToPtr(ptr, ctx.Types.i64Ptr);
	ptr = ctx.GetBuilder().CreateLoad(ptr);
	ptr = ctx.GetBuilder().CreateAdd(ptr, page_offset);
	ptr = ctx.GetBuilder().CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8));
	llvm::Value *match_value = ctx.GetBuilder().CreateLoad(ptr);

	if(archsim::options::Verbose) {
		EmitIncrementCounter(ctx, ctx.GetThread()->GetMetrics().ReadHits);
	}

	ctx.GetBuilder().CreateBr(continue_block);
	ctx.GetBuilder().SetInsertPoint(nomatch_block);

#endif
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


	llvm::Value *nomatch_value = ctx.GetBuilder().CreateCall(fn_ptr, {ctx.Values.state_block_ptr, address, llvm::ConstantInt::get(ctx.Types.i32, interface)});

#ifdef FAST_READS
	ctx.GetBuilder().CreateBr(continue_block);

	ctx.GetBuilder().SetInsertPoint(continue_block);
	auto value = ctx.GetBuilder().CreatePHI(llvm::IntegerType::getIntNTy(ctx.LLVMCtx, size_in_bytes*8), 2);
	value->addIncoming(match_value, match_block);
	value->addIncoming(nomatch_value, nomatch_block);
#else
	auto value = nomatch_value;
#endif
#endif
	if(archsim::options::Trace) {
		ctx.GetBuilder().CreateCall(trace_fn_ptr, {ctx.GetThreadPtr(), address, value});
	}

	return value;
}

void BaseLLVMTranslate::EmitMemoryWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value* address, llvm::Value* value)
{
#ifdef FASTER_READS
	llvm::Value *mem_offset = llvm::ConstantInt::get(ctx.Types.i64, 0x80000000);
	address = ctx.GetBuilder().CreateAdd(address, mem_offset);
	llvm::Value *ptr = ctx.GetBuilder().CreateCast(llvm::Instruction::IntToPtr, address, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8, 0));
	if(ptr->getValueID() == llvm::Instruction::InstructionVal + llvm::Instruction::IntToPtr) {
		((llvm::IntToPtrInst*)ptr)->setMetadata("aaai", llvm::MDNode::get(ctx.LLVMCtx, { llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, archsim::translate::translate_llvm::TAG_MEM_ACCESS)) }));
	}

	ctx.GetBuilder().CreateStore(value, ptr);

	llvm::Function *trace_fn_ptr = nullptr;
	switch(size_in_bytes) {
		case 1:
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite8;
			break;
		case 2:
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite16;
			break;
		case 4:
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite32;
			break;
		case 8:
			trace_fn_ptr = ctx.Functions.cpuTraceMemWrite64;
			break;
		default:
			UNIMPLEMENTED;
	}
#else
#ifdef FAST_WRITES
	llvm::Value *cache_ptr = ctx.GetBuilder().CreatePtrToInt(ctx.GetStateBlockPointer("memory_cache_" + std::to_string(interface)), ctx.Types.i64);
	llvm::Value *page_index = ctx.GetBuilder().CreateLShr(address, 12);

	if(archsim::options::Verbose) {
		EmitIncrementCounter(ctx, ctx.GetThread()->GetMetrics().Writes);
	}

	// TODO: get rid of these magic numbers (number of entries in cache, and cache entry size)
	llvm::Value *cache_entry = ctx.GetBuilder().CreateURem(page_index, llvm::ConstantInt::get(ctx.Types.i64, 1024));
	cache_entry = ctx.GetBuilder().CreateMul(cache_entry, llvm::ConstantInt::get(ctx.Types.i64, 16));
	cache_entry = ctx.GetBuilder().CreateAdd(cache_ptr, cache_entry);

	llvm::Value *tag = ctx.GetBuilder().CreateIntToPtr(cache_entry, ctx.Types.i64Ptr);
	tag = ctx.GetBuilder().CreateLoad(tag);

	// does tag match?
	llvm::Value *offset_page_base = ctx.GetBuilder().CreateAdd(address, llvm::ConstantInt::get(ctx.Types.i64, size_in_bytes-1));
	offset_page_base = ctx.GetBuilder().CreateAnd(offset_page_base, ~archsim::Address::PageMask);

	llvm::Value *tag_match = ctx.GetBuilder().CreateICmpEQ(tag, offset_page_base);
	llvm::BasicBlock *match_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	llvm::BasicBlock *nomatch_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	auto br = ctx.GetBuilder().CreateCondBr(tag_match, match_block, nomatch_block);
	auto mdnode = llvm::MDNode::get(ctx.LLVMCtx, {llvm::MDString::get(ctx.LLVMCtx, "branch_weights"), llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, 95)), llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, 5))});
	br->setMetadata(llvm::LLVMContext::MD_prof, mdnode);

	ctx.GetBuilder().SetInsertPoint(match_block);

	llvm::Value *page_offset = ctx.GetBuilder().CreateAnd(address, archsim::Address::PageMask);
	llvm::Value *ptr = ctx.GetBuilder().CreateAdd(cache_entry, llvm::ConstantInt::get(ctx.Types.i64, 8));
	ptr = ctx.GetBuilder().CreateIntToPtr(ptr, ctx.Types.i64Ptr);
	ptr = ctx.GetBuilder().CreateLoad(ptr);
	ptr = ctx.GetBuilder().CreateAdd(ptr, page_offset);
	ptr = ctx.GetBuilder().CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8));
	ctx.GetBuilder().CreateStore(value, ptr);

	if(archsim::options::Verbose) {
		EmitIncrementCounter(ctx, ctx.GetThread()->GetMetrics().WriteHits);
	}

	ctx.GetBuilder().CreateBr(continue_block);


	ctx.GetBuilder().SetInsertPoint(nomatch_block);
#endif

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

	ctx.GetBuilder().CreateCall(fn_ptr, {GetThreadPtr(ctx), llvm::ConstantInt::get(ctx.Types.i32, interface), address, value});

#ifdef FAST_WRITES
	ctx.GetBuilder().CreateBr(continue_block);

	ctx.GetBuilder().SetInsertPoint(continue_block);
#endif
#endif
	if(archsim::options::Trace) {
		ctx.GetBuilder().CreateCall(trace_fn_ptr, {ctx.GetThreadPtr(), address, value});
	}
}

void BaseLLVMTranslate::EmitTakeException(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, llvm::Value* category, llvm::Value* data)
{
	ctx.GetBuilder().CreateCall(ctx.Functions.TakeException, {GetThreadPtr(ctx), category, data});
}

llvm::Value* BaseLLVMTranslate::GetRegisterPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset)
{
	llvm::Value *ptr = GetRegfilePtr(ctx);
	ptr = ctx.GetBuilder().CreatePtrToInt(ptr, ctx.Types.i64);
	ptr = ctx.GetBuilder().CreateAdd(ptr, llvm::ConstantInt::get(ctx.Types.i64, offset));
	ptr = ctx.GetBuilder().CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(ctx.LLVMCtx, size_in_bytes*8));
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
	// TODO: this leads to not very efficient code. It would be better to
	// figure out a better way of handling this, possibly by modifying LLVM
	// (new intrinsic or custom lowering)

	llvm::IntegerType *base_itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits);
	llvm::IntegerType *itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits*2);

	lhs = ctx.GetBuilder().CreateZExtOrTrunc(lhs, base_itype);
	rhs = ctx.GetBuilder().CreateZExtOrTrunc(rhs, base_itype);
	carry = ctx.GetBuilder().CreateZExtOrTrunc(carry, base_itype);

	// cast each value to correct bitwidth
	llvm::Value *extended_lhs = ctx.GetBuilder().CreateZExtOrTrunc(lhs, itype);
	llvm::Value *extended_rhs = ctx.GetBuilder().CreateZExtOrTrunc(rhs, itype);
	llvm::Value *extended_carry = ctx.GetBuilder().CreateZExtOrTrunc(carry, itype);

	// produce result
	llvm::Value *extended_result = ctx.GetBuilder().CreateAdd(extended_rhs, extended_carry);
	extended_result = ctx.GetBuilder().CreateAdd(extended_lhs, extended_result);

	llvm::Value *base_result = ctx.GetBuilder().CreateTrunc(extended_result, base_itype);

	// calculate flags
	llvm::Value *Z = ctx.GetBuilder().CreateICmpEQ(base_result, llvm::ConstantInt::get(base_itype, 0));
	Z = ctx.GetBuilder().CreateZExt(Z, ctx.Types.i8);

	llvm::Value *N = ctx.GetBuilder().CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));
	N = ctx.GetBuilder().CreateZExt(N, ctx.Types.i8);

	llvm::Value *C = ctx.GetBuilder().CreateICmpUGT(extended_result, ctx.GetBuilder().CreateZExt(llvm::ConstantInt::get(base_itype, -1ull), itype));
	C = ctx.GetBuilder().CreateZExt(C, ctx.Types.i8);

	llvm::Value *lhs_sign = ctx.GetBuilder().CreateICmpSLT(lhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *rhs_sign = ctx.GetBuilder().CreateICmpSLT(rhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *result_sign = ctx.GetBuilder().CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));

	llvm::Value *V_lhs = ctx.GetBuilder().CreateICmpEQ(lhs_sign, rhs_sign);
	llvm::Value *V_rhs = ctx.GetBuilder().CreateICmpNE(lhs_sign, result_sign);

	llvm::Value *V = ctx.GetBuilder().CreateAnd(V_lhs, V_rhs);
	V = ctx.GetBuilder().CreateZExt(V, ctx.Types.i8);

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
	// TODO: this leads to not very efficient code. It would be better to
	// figure out a better way of handling this, possibly by modifying LLVM
	// (new intrinsic or custom lowering)

	llvm::IntegerType *base_itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits);
	llvm::IntegerType *itype = llvm::IntegerType::getIntNTy(ctx.LLVMCtx, bits*2);

	lhs = ctx.GetBuilder().CreateZExtOrTrunc(lhs, base_itype);
	rhs = ctx.GetBuilder().CreateZExtOrTrunc(rhs, base_itype);
	carry = ctx.GetBuilder().CreateZExtOrTrunc(carry, base_itype);

	// cast each value to correct bitwidth
	llvm::Value *extended_lhs = ctx.GetBuilder().CreateZExtOrTrunc(lhs, itype);
	llvm::Value *extended_rhs = ctx.GetBuilder().CreateZExtOrTrunc(rhs, itype);
	llvm::Value *extended_carry = ctx.GetBuilder().CreateZExtOrTrunc(carry, itype);

	// produce result
	llvm::Value *result = ctx.GetBuilder().CreateAdd(extended_rhs, extended_carry);
	result = ctx.GetBuilder().CreateSub(extended_lhs, result);

	llvm::Value *base_result = ctx.GetBuilder().CreateTrunc(result, base_itype);

	// calculate flags
	llvm::Value *Z = ctx.GetBuilder().CreateICmpEQ(base_result, llvm::ConstantInt::get(base_itype, 0));
	Z = ctx.GetBuilder().CreateZExt(Z, ctx.Types.i8);

	llvm::Value *N = ctx.GetBuilder().CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));
	N = ctx.GetBuilder().CreateZExt(N, ctx.Types.i8);

	llvm::Value *C = ctx.GetBuilder().CreateICmpUGT(result, ctx.GetBuilder().CreateZExt(llvm::ConstantInt::get(base_itype, -1ull), itype));
	C = ctx.GetBuilder().CreateZExt(C, ctx.Types.i8);

	llvm::Value *lhs_sign = ctx.GetBuilder().CreateICmpSLT(lhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *rhs_sign = ctx.GetBuilder().CreateICmpSLT(rhs, llvm::ConstantInt::get(base_itype, 0));
	llvm::Value *result_sign = ctx.GetBuilder().CreateICmpSLT(base_result, llvm::ConstantInt::get(base_itype, 0));

	llvm::Value *V_lhs = ctx.GetBuilder().CreateICmpNE(lhs_sign, rhs_sign);
	llvm::Value *V_rhs = ctx.GetBuilder().CreateICmpEQ(rhs_sign, result_sign);

	llvm::Value *V = ctx.GetBuilder().CreateAnd(V_lhs, V_rhs);
	V = ctx.GetBuilder().CreateZExt(V, ctx.Types.i8);

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
	dynamic_blocks[queued_block] = llvm::BasicBlock::Create(ctx.LLVMCtx, "", ctx.GetBuilder().GetInsertBlock()->getParent());
	dynamic_block_queue.push_back(queued_block);
}

void BaseLLVMTranslate::EmitTraceBankedRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* regnum, int size, llvm::Value* value_ptr)
{
	if(archsim::options::Trace) {
		ctx.GetBuilder().CreateCall(ctx.Functions.cpuTraceBankedRegisterWrite, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.GetBuilder().CreateZExtOrTrunc(regnum, ctx.Types.i32), llvm::ConstantInt::get(ctx.Types.i32, size), value_ptr});
	}
}

void BaseLLVMTranslate::EmitTraceRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* value)
{
	if(archsim::options::Trace) {
		ctx.GetBuilder().CreateCall(ctx.Functions.cpuTraceRegisterWrite, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.GetBuilder().CreateZExtOrTrunc(value, ctx.Types.i64)});
	}
}

void BaseLLVMTranslate::EmitTraceBankedRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* regnum, int size, llvm::Value* value_ptr)
{
	if(archsim::options::Trace) {
		ctx.GetBuilder().CreateCall(ctx.Functions.cpuTraceBankedRegisterRead, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.GetBuilder().CreateZExtOrTrunc(regnum, ctx.Types.i32), llvm::ConstantInt::get(ctx.Types.i32, size), value_ptr});
	}
}

void BaseLLVMTranslate::EmitTraceRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value* value)
{
	if(archsim::options::Trace) {
		ctx.GetBuilder().CreateCall(ctx.Functions.cpuTraceRegisterRead, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i32, id), ctx.GetBuilder().CreateZExtOrTrunc(value, ctx.Types.i64)});
	}
}

void BaseLLVMTranslate::EmitIncrementCounter(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, archsim::util::Counter64& counter, uint32_t value)
{
	llvm::Value *ptr = llvm::ConstantInt::get(ctx.Types.i64, (uint64_t)counter.get_ptr());
	ptr = ctx.GetBuilder().CreateIntToPtr(ptr, ctx.Types.i64Ptr);
	llvm::Value *val = ctx.GetBuilder().CreateLoad(ptr);
	val = ctx.GetBuilder().CreateAdd(val, llvm::ConstantInt::get(ctx.Types.i64, value));
	ctx.GetBuilder().CreateStore(val, ptr);
}
