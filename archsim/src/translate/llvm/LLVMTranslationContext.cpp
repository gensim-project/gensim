/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/profile/Region.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"
#include "wutils/vbitset.h"

using namespace archsim::translate::translate_llvm;


LLVMRegionTranslationContext::LLVMRegionTranslationContext(LLVMTranslationContext &ctx, llvm::Function *fn, translate::TranslationWorkUnit &work_unit, llvm::BasicBlock *entry_block, gensim::BaseLLVMTranslate *txlt) : ctx_(ctx), function_(fn), entry_block_(entry_block), txlt_(txlt), region_chain_block_(nullptr)
{
	BuildDispatchBlock(work_unit);
}

void LLVMRegionTranslationContext::BuildJumpTable(TranslationWorkUnit& work_unit)
{
	translate_llvm::LLVMTranslationContext ctx(GetFunction()->getContext(), GetFunction(), work_unit.GetThread());
	llvm::IRBuilder<> builder(GetDispatchBlock());
//	ctx.SetBuilder(builder);

	auto arraytype = llvm::ArrayType::get(ctx.Types.i8Ptr, 4096);
	std::map<archsim::Address, llvm::BlockAddress*> ptrs;

	std::set<llvm::BasicBlock*> entry_blocks;
	for(auto b : work_unit.GetBlocks()) {
		if(b.second->IsEntryBlock()) {
			entry_blocks.insert(GetBlockMap().at(b.first));
			auto block_address = llvm::BlockAddress::get(GetFunction(), GetBlockMap().at(b.first));
			ptrs[b.first.PageOffset()] = block_address;
		}
	}

	auto exit_block = GetExitBlock(EXIT_REASON_NOBLOCK);
	auto exit_block_ptr = llvm::BlockAddress::get(GetFunction(), exit_block);

	std::vector<llvm::Constant *> constants;
	for(archsim::Address a = archsim::Address(0); a < archsim::Address(0x1000); a += 1) {
		if(ptrs.count(a.PageOffset())) {
			constants.push_back(ptrs.at(a));
		} else {
			constants.push_back(exit_block_ptr);
		}
	}

	llvm::ConstantArray *block_jump_table = (llvm::ConstantArray*)llvm::ConstantArray::get(arraytype, constants);

	llvm::Constant *constant = GetFunction()->getParent()->getOrInsertGlobal("jump_table", arraytype);
	llvm::GlobalVariable *gv = GetFunction()->getParent()->getGlobalVariable("jump_table");
	gv->setConstant(true);
	gv->setInitializer(block_jump_table);
	gv->setMetadata("aaai", llvm::MDNode::get(ctx.LLVMCtx, { llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, archsim::translate::translate_llvm::TAG_JT_ELEMENT)) }));

	llvm::Value *pc = GetContext().LoadGuestRegister(builder, GetContext().GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);

	// need to make sure we're still on the right page
	auto pc_check_less = builder.CreateICmpULT(pc, llvm::ConstantInt::get(ctx.Types.i64, work_unit.GetRegion().GetPhysicalBaseAddress().Get()));
	auto pc_check_greater = builder.CreateICmpUGE(pc, llvm::ConstantInt::get(ctx.Types.i64, work_unit.GetRegion().GetPhysicalBaseAddress().Get() + 4096));
	auto pc_check = builder.CreateOr(pc_check_less, pc_check_greater);

	llvm::BasicBlock *on_page_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "jump_table_block", GetFunction());
	builder.CreateCondBr(pc_check, exit_block, on_page_block);

	builder.SetInsertPoint(on_page_block);

	auto pc_offset = builder.CreateAnd(pc, archsim::Address::PageMask);
	auto target = builder.CreateGEP(gv, {llvm::ConstantInt::get(ctx.Types.i64, 0), pc_offset});
	if(llvm::isa<llvm::GetElementPtrInst>(target)) {
		((llvm::GetElementPtrInst*)target)->setMetadata("aaai", llvm::MDNode::get(ctx.LLVMCtx, { llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, archsim::translate::translate_llvm::TAG_JT_ELEMENT)) }));
	}
	llvm::LoadInst *linst = builder.CreateLoad(target);
	// give target aaai metadata as though it was a jump table entry - it's certainly not going to alias with anything else.
	linst->setMetadata("aaai", llvm::MDNode::get(ctx.LLVMCtx, { llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(ctx.Types.i32, archsim::translate::translate_llvm::TAG_JT_ELEMENT)) }));
	auto indirectbr = builder.CreateIndirectBr(linst);

	for(auto i : entry_blocks) {
		indirectbr->addDestination(i);
	}
	indirectbr->addDestination(exit_block);
}

void LLVMRegionTranslationContext::BuildSwitchStatement(TranslationWorkUnit& work_unit)
{
	std::set<archsim::Address> entry_blocks;
	for(auto b : work_unit.GetBlocks()) {
		if(b.second->IsEntryBlock()) {
			entry_blocks.insert(b.first);
		}
	}

	llvm::IRBuilder<> builder(GetDispatchBlock());
	translate_llvm::LLVMTranslationContext ctx(GetFunction()->getContext(), GetFunction(), work_unit.GetThread());
//	ctx.SetBuilder(builder);

	// TODO: figure out from architecture
	llvm::Value *pc = GetContext().LoadGuestRegister(builder, GetContext().GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);

	auto switch_inst = builder.CreateSwitch(pc, GetExitBlock(EXIT_REASON_NOBLOCK), GetBlockMap().size());
	for(auto block : GetBlockMap()) {
		if(entry_blocks.count(block.first)) {
			switch_inst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(GetFunction()->getContext()), work_unit.GetRegion().GetPhysicalBaseAddress().Get() + block.first.GetPageOffset()), block.second);
		}
	}
}

llvm::BasicBlock *LLVMRegionTranslationContext::GetRegionChainBlock()
{
	if(region_chain_block_ == nullptr) {

		llvm::Type *chain_entry_type = llvm::StructType::get(GetContext().LLVMCtx, {GetContext().Types.i64, GetFunction()->getType()});

		region_chain_block_ = llvm::BasicBlock::Create(GetContext().LLVMCtx, "chain_block", GetFunction());
		llvm::IRBuilder<> builder (region_chain_block_);
		// TODO: figure out from architecture
		llvm::Value *pc = GetContext().LoadGuestRegister(builder, GetContext().GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);

		auto page_index = builder.CreateLShr(pc, 12);
		auto page_base = builder.CreateAnd(pc, ~archsim::Address::PageMask);
		auto cache_base_ptr = GetContext().GetStateBlockPointer(builder, "page_cache");

		cache_base_ptr = builder.CreateBitCast(cache_base_ptr, chain_entry_type->getPointerTo()->getPointerTo());
		llvm::LoadInst *cache_base = builder.CreateLoad(cache_base_ptr);
		cache_base->setMetadata("aaai", llvm::MDNode::get(GetContext().LLVMCtx, {llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(GetContext().Types.i64, archsim::translate::translate_llvm::TAG_REGION_CHAIN_TABLE))}));

		auto cache_index = builder.CreateURem(page_index, llvm::ConstantInt::get(GetContext().Types.i64, 1024));

		auto cache_tag = builder.CreateGEP(cache_base, {cache_index, llvm::ConstantInt::get(GetContext().Types.i32, 0)});
		//cache_tag->setMetadata("aaai", llvm::MDNode::get(GetContext().LLVMCtx, {llvm::ConstantAsMetadata(llvm::ConstantInt::get(GetContext().Types.i64, TAG_REGION_CHAIN_TABLE))}));
		cache_tag = builder.CreateLoad(cache_tag);

		auto tag_matches = builder.CreateICmpEQ(page_base, cache_tag);

		llvm::BasicBlock *match_block = llvm::BasicBlock::Create(GetContext().LLVMCtx, "chain_success", GetFunction());
		llvm::BasicBlock *nomatch_block = llvm::BasicBlock::Create(GetContext().LLVMCtx, "chain_fail", GetFunction());

		builder.CreateCondBr(tag_matches, match_block, nomatch_block);

		builder.SetInsertPoint(match_block);

		if(archsim::options::Verbose) {
			GetTxlt()->EmitIncrementCounter(builder, GetContext(), GetContext().GetThread()->GetMetrics().JITSuccessfulChains, 1);
		}

		auto fn_ptr = builder.CreateGEP(cache_base, {cache_index, llvm::ConstantInt::get(GetContext().Types.i32, 1)});
		//fn_ptr->setMetadata("aaai", llvm::MDNode::get(GetContext().LLVMCtx, {llvm::ConstantAsMetadata(llvm::ConstantInt::get(GetContext().Types.i64, TAG_REGION_CHAIN_TABLE))}));
		fn_ptr = builder.CreateLoad(fn_ptr);

		auto result = builder.CreateCall(fn_ptr, {GetContext().GetRegStatePtr(), GetContext().GetStateBlockPointer()});
		builder.CreateRet(result);

		builder.SetInsertPoint(nomatch_block);

		if(archsim::options::Verbose) {
			GetTxlt()->EmitIncrementCounter(builder, GetContext(), GetContext().GetThread()->GetMetrics().JITFailedChains, 1);
		}
		builder.CreateRet(llvm::ConstantInt::get(GetContext().Types.i32, 1));

	}

	return region_chain_block_;
}

void LLVMRegionTranslationContext::BuildDispatchBlock(TranslationWorkUnit& work_unit)
{
	auto *entry_block = &GetFunction()->getEntryBlock();
	for(auto b : work_unit.GetBlocks()) {
		auto block_block = llvm::BasicBlock::Create(GetFunction()->getContext(), "block_" + std::to_string(b.first.Get()), GetFunction());
		GetBlockMap()[b.first] = block_block;
	}

	dispatch_block_ = llvm::BasicBlock::Create(GetFunction()->getContext(), "dispatch", GetFunction());
	llvm::BranchInst::Create(dispatch_block_, entry_block);

// 	BuildSwitchStatement(work_unit);
	BuildJumpTable(work_unit);
}

llvm::BasicBlock* LLVMRegionTranslationContext::GetExitBlock(int exit_reason)
{
	if(exit_reason == EXIT_REASON_NEXTPAGE) {
		return GetRegionChainBlock();
	}

	if(!exit_blocks_.count(exit_reason)) {
		auto block = llvm::BasicBlock::Create(GetContext().LLVMCtx,"exit_block_" + std::to_string(exit_reason), GetFunction());
		exit_blocks_[exit_reason] = block;
		llvm::ReturnInst::Create(GetContext().LLVMCtx, llvm::ConstantInt::get(GetContext().Types.i32, exit_reason), block);
	}

	return exit_blocks_.at(exit_reason);
}

bool ArchHasOverlappingRegs(const archsim::ArchDescriptor &arch)
{
	const auto &reg_file_descriptor = arch.GetRegisterFileDescriptor();

	// map extents of each register view to the view descriptor
	std::map<std::pair<uint32_t, uint32_t>, std::vector<const archsim::RegisterFileEntryDescriptor*>> reg_file_entries;
	for(auto i : reg_file_descriptor.GetEntries()) {
		reg_file_entries[ {i.second.GetOffset(), i.second.GetOffset() + (i.second.GetEntryCountPerRegister() * i.second.GetEntryStride()) - 1}].push_back(&i.second);
	}

	// figure out if any extents overlap
	wutils::vbitset<> extent_bits (reg_file_descriptor.GetSize());

	for(auto i : reg_file_entries) {
		for(int bit = i.first.first; bit <= i.first.second; ++bit) {
			if(extent_bits.get(bit)) {
				return true;
			}
			extent_bits.set(bit, 1);
		}
	}

	return false;
}


LLVMTranslationContext::LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::Function *fn, archsim::core::thread::ThreadInstance *thread) : LLVMCtx(ctx), thread_(thread), Module(fn->getParent()), function_(fn), prev_reg_block_(nullptr)
{
	Values.reg_file_ptr = fn->arg_begin();
	Values.state_block_ptr = fn->arg_begin()+1;

	Types.vtype  = llvm::Type::getVoidTy(ctx);
	Types.i1  = llvm::IntegerType::getInt1Ty(ctx);
	Types.i8  = llvm::IntegerType::getInt8Ty(ctx);
	Types.i8Ptr  = llvm::IntegerType::getInt8PtrTy(ctx, 0);
	Types.i16 = llvm::IntegerType::getInt16Ty(ctx);
	Types.i32 = llvm::IntegerType::getInt32Ty(ctx);
	Types.i64 = llvm::IntegerType::getInt64Ty(ctx);
	Types.i64Ptr = llvm::IntegerType::getInt64PtrTy(ctx);
	Types.i128 = llvm::IntegerType::getInt128Ty(ctx);

	Types.f32 = llvm::Type::getFloatTy(ctx);
	Types.f64 = llvm::Type::getDoubleTy(ctx);

	auto memory_type = llvm::ArrayType::get(Types.i8, ~(0ULL))->getPointerTo();
	fn->getParent()->getOrInsertGlobal("contiguous_mem_base", memory_type);
	auto mem_base_global = fn->getParent()->getGlobalVariable("contiguous_mem_base");
	mem_base_global->setConstant(true);
	mem_base_global->setInitializer(llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(Types.i64, 0x80000000), memory_type));

	if(fn->getEntryBlock().size()) {
		Values.contiguous_mem_base = new llvm::LoadInst(mem_base_global, "", &*(fn->getEntryBlock().begin()));
// 		Values.contiguous_mem_base = new llvm::IntToPtrInst(llvm::ConstantInt::get(Types.i64, 0x80000000), Types.i8Ptr, "contiguous_mem_base", &*(fn->getEntryBlock().begin()));
	} else {
		Values.contiguous_mem_base = new llvm::LoadInst(mem_base_global, "", &(fn->getEntryBlock()));
// 		Values.contiguous_mem_base = new llvm::IntToPtrInst(llvm::ConstantInt::get(Types.i64, 0x80000000), Types.i8Ptr, "contiguous_mem_base", &(fn->getEntryBlock()));
	}

	Functions.ctpop_i32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ctpop, Types.i32);
	Functions.bswap_i32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::bswap, Types.i32);
	Functions.bswap_i64 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::bswap, Types.i64);

	Functions.float_sqrt = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::sqrt, Types.f32);
	Functions.double_sqrt = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::sqrt, Types.f64);

	Functions.jit_trap =  (llvm::Function*)Module->getOrInsertFunction("cpuTrap", Types.vtype, Types.i8Ptr);

	Functions.assume = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::assume);
	Functions.debug_trap = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::debugtrap);

	Functions.uadd_with_overflow_8 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::uadd_with_overflow, Types.i8);
	Functions.uadd_with_overflow_16 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::uadd_with_overflow, Types.i16);
	Functions.uadd_with_overflow_32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::uadd_with_overflow, Types.i32);
	Functions.uadd_with_overflow_64 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::uadd_with_overflow, Types.i64);

	Functions.usub_with_overflow_8 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::usub_with_overflow, Types.i8);
	Functions.usub_with_overflow_16 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::usub_with_overflow, Types.i16);
	Functions.usub_with_overflow_32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::usub_with_overflow, Types.i32);
	Functions.usub_with_overflow_64 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::usub_with_overflow, Types.i64);

	Functions.sadd_with_overflow_8 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::sadd_with_overflow, Types.i8);
	Functions.sadd_with_overflow_16 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::sadd_with_overflow, Types.i16);
	Functions.sadd_with_overflow_32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::sadd_with_overflow, Types.i32);
	Functions.sadd_with_overflow_64 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::sadd_with_overflow, Types.i64);

	Functions.ssub_with_overflow_8 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ssub_with_overflow, Types.i8);
	Functions.ssub_with_overflow_16 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ssub_with_overflow, Types.i16);
	Functions.ssub_with_overflow_32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ssub_with_overflow, Types.i32);
	Functions.ssub_with_overflow_64 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ssub_with_overflow, Types.i64);

	Functions.blkRead8 =  (llvm::Function*)Module->getOrInsertFunction("blkRead8", Types.i8, Types.i8Ptr, Types.i64, Types.i32);
	Functions.blkRead16 = (llvm::Function*)Module->getOrInsertFunction("blkRead16", Types.i16, Types.i8Ptr, Types.i64, Types.i32);
	Functions.blkRead32 = (llvm::Function*)Module->getOrInsertFunction("blkRead32", Types.i32, Types.i8Ptr, Types.i64, Types.i32);
	Functions.blkRead64 = (llvm::Function*)Module->getOrInsertFunction("blkRead64", Types.i64, Types.i8Ptr, Types.i64, Types.i32);

	Functions.cpuWrite8 =  (llvm::Function*)Module->getOrInsertFunction("cpuWrite8", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i8);
	Functions.cpuWrite16 = (llvm::Function*)Module->getOrInsertFunction("cpuWrite16", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i16);
	Functions.cpuWrite32 = (llvm::Function*)Module->getOrInsertFunction("cpuWrite32", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i32);
	Functions.cpuWrite64 = (llvm::Function*)Module->getOrInsertFunction("cpuWrite64", Types.vtype, Types.i8Ptr, Types.i32, Types.i64, Types.i64);

	Functions.cpuTraceMemWrite8 =  (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite8", Types.vtype, Types.i8Ptr, Types.i64, Types.i8);
	Functions.cpuTraceMemWrite16 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite16", Types.vtype, Types.i8Ptr, Types.i64, Types.i16);
	Functions.cpuTraceMemWrite32 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite32", Types.vtype, Types.i8Ptr, Types.i64, Types.i32);
	Functions.cpuTraceMemWrite64 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemWrite64", Types.vtype, Types.i8Ptr, Types.i64, Types.i64);

	Functions.cpuTraceMemRead8 =  (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead8", Types.vtype, Types.i8Ptr, Types.i64, Types.i8);
	Functions.cpuTraceMemRead16 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead16", Types.vtype, Types.i8Ptr, Types.i64, Types.i16);
	Functions.cpuTraceMemRead32 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead32", Types.vtype, Types.i8Ptr, Types.i64, Types.i32);
	Functions.cpuTraceMemRead64 = (llvm::Function*)Module->getOrInsertFunction("cpuTraceOnlyMemRead64", Types.vtype, Types.i8Ptr, Types.i64, Types.i64);

	Functions.cpuTraceBankedRegisterWrite = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegBankWrite", Types.vtype, Types.i8Ptr, Types.i32, Types.i32, Types.i32, Types.i8Ptr);
	Functions.cpuTraceRegisterWrite = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegWrite", Types.vtype, Types.i8Ptr, Types.i32, Types.i64);
	Functions.cpuTraceBankedRegisterRead = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegBankRead", Types.vtype, Types.i8Ptr, Types.i32, Types.i32, Types.i32, Types.i8Ptr);
	Functions.cpuTraceRegisterRead = (llvm::Function*)Module->getOrInsertFunction("cpuTraceRegRead", Types.vtype, Types.i8Ptr, Types.i32, Types.i64);

	Functions.TakeException = (llvm::Function*)Module->getOrInsertFunction("cpuTakeException", Types.vtype, Types.i8Ptr, Types.i32, Types.i32);

	Functions.cpuTraceInstruction = (llvm::Function*)Module->getOrInsertFunction("cpuTraceInstruction", Types.vtype, Types.i8Ptr, Types.i64, Types.i32, Types.i32, Types.i32, Types.i32);
	Functions.cpuTraceInsnEnd = (llvm::Function*)Module->getOrInsertFunction("cpuTraceInsnEnd", Types.vtype, Types.i8Ptr);

	guest_reg_emitter_ = new GEPLLVMGuestRegisterAccessEmitter(*this);
}

llvm::Value* LLVMTranslationContext::GetThreadPtr(llvm::IRBuilder<> &builder)
{
	auto ptr = Values.state_block_ptr;
	return builder.CreateLoad(builder.CreatePointerCast(ptr, Types.i8Ptr->getPointerTo(0)));
}

llvm::Value* LLVMTranslationContext::GetRegStatePtr()
{
	return Values.reg_file_ptr;
}

llvm::Function* LLVMTranslationContext::GetFunction()
{
	return function_;
}


llvm::Value* LLVMTranslationContext::GetStateBlockPointer(llvm::IRBuilder<> &builder, const std::string& entry)
{
	auto ptr = Values.state_block_ptr;
	ptr = builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(Types.i64, thread_->GetStateBlock().GetBlockOffset(entry))});
	return ptr;
}

llvm::Value* LLVMTranslationContext::GetStateBlockPointer()
{
	return Values.state_block_ptr;
}


llvm::Value * archsim::translate::translate_llvm::LLVMTranslationContext::GetTraceStackSlot(llvm::Type* type)
{
	if(!trace_slots_.count(type)) {
		trace_slots_[type] = new llvm::AllocaInst(type, 0, "", &*GetFunction()->getEntryBlock().begin());
	}

	return trace_slots_.at(type);
}

llvm::Value* LLVMTranslationContext::AllocateRegister(llvm::Type *type, int name)
{
	if(free_registers_[type].empty()) {
		auto &block = GetFunction()->getEntryBlock();

		llvm::Value *new_reg = nullptr;

		if(block.empty()) {
			new_reg = new llvm::AllocaInst(type, 0, nullptr, "", &block);
		} else {
			new_reg = new llvm::AllocaInst(type, 0, nullptr, "", &*block.begin());
		}

		allocated_registers_[type].push_back(new_reg);
		live_register_pointers_[name] = new_reg;

		return new_reg;
	} else {
		auto reg = free_registers_[type].back();
		free_registers_[type].pop_back();

		allocated_registers_[type].push_back(reg);
		live_register_pointers_[name] = reg;

		return reg;
	}
}

void LLVMTranslationContext::FreeRegister(llvm::Type *t, llvm::Value* v, int name)
{
	UNIMPLEMENTED;
//	free_registers_[t].push_back(v);
}

void LLVMTranslationContext::ResetRegisters()
{
	for(auto &i : allocated_registers_) {
		for(auto &j : i.second) {
			free_registers_[i.first].push_back(j);
		}
	}
	allocated_registers_.clear();
	live_register_pointers_.clear();
	live_register_values_.clear();
}

llvm::Value *LLVMTranslationContext::GetRegPtr(llvm::IRBuilder<> &builder, llvm::Value *offset, llvm::Type *type)
{
	llvm::Value *ptr = (llvm::Instruction*)builder.CreatePtrToInt(GetRegStatePtr(), Types.i64);
	ptr = builder.CreateAdd(ptr, offset);
	ptr = builder.CreateIntToPtr(ptr, type->getPointerTo(0));
	((llvm::Instruction*)ptr)->setMetadata("aaai", llvm::MDNode::get(LLVMCtx, {
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, archsim::translate::translate_llvm::TAG_REG_ACCESS))
	}
	                                                                ));
	return ptr;
}

llvm::Value* LLVMTranslationContext::GetRegPtr(llvm::IRBuilder<> &builder, int offset, llvm::Type* type)
{
#define INTERN_REG_PTRS
#ifdef INTERN_REG_PTRS
	if(guest_reg_ptrs_.count({offset, type}) == 0) {

		// insert pointer calculation into header block
		// ptrtoint
		auto insert_point = &*GetFunction()->getEntryBlock().begin();

		llvm::IRBuilder<> sub_builder(insert_point);
		llvm::Value *ptr = (llvm::Instruction*)sub_builder.CreatePtrToInt(GetRegStatePtr(), Types.i64);
		ptr = sub_builder.CreateAdd(ptr, llvm::ConstantInt::get(Types.i64, offset));
		ptr = sub_builder.CreateIntToPtr(ptr, type->getPointerTo(0));

		((llvm::Instruction*)ptr)->setMetadata("aaai", llvm::MDNode::get(LLVMCtx, {
			llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, archsim::translate::translate_llvm::TAG_REG_ACCESS)),
			llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, offset)),
			llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, offset + type->getScalarSizeInBits()/8)),
			llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, type->getScalarSizeInBits()/8))
		}
		                                                                ));

		guest_reg_ptrs_[ {offset, type}] = ptr;
	}

	return guest_reg_ptrs_.at({offset, type});
#else
	llvm::Value *ptr = (llvm::Instruction*)builder.CreatePtrToInt(GetRegStatePtr(), Types.i64);
	ptr = builder.CreateAdd(ptr, llvm::ConstantInt::get(Types.i64, offset));
	ptr = builder.CreateIntToPtr(ptr, type->getPointerTo(0));
	((llvm::Instruction*)ptr)->setMetadata("aaai", llvm::MDNode::get(LLVMCtx, {
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, archsim::translate::translate_llvm::TAG_REG_ACCESS)),
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, offset)),
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, offset + type->getScalarSizeInBits()/8)),
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(Types.i32, type->getScalarSizeInBits()/8))
	}
	                                                                ));

	return ptr;
#endif
}

llvm::Value* LLVMTranslationContext::LoadGuestRegister(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	return guest_reg_emitter_->EmitRegisterRead(builder, reg, index);
}

void LLVMTranslationContext::StoreGuestRegister(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	return guest_reg_emitter_->EmitRegisterWrite(builder, reg, index, value);
}


llvm::Value* LLVMTranslationContext::LoadRegister(llvm::IRBuilder<> &builder, int name)
{
//	return builder.CreateLoad(live_register_pointers_.at(name));

	if(!live_register_values_.count(name)) {
		live_register_values_[name] = builder.CreateLoad(live_register_pointers_.at(name));
	}
	return live_register_values_.at(name);
}

void LLVMTranslationContext::StoreRegister(llvm::IRBuilder<> &builder, int name, llvm::Value* value)
{
//	builder.CreateStore(value, live_register_pointers_.at(name));
	live_register_values_[name] = value;
}

void LLVMTranslationContext::ResetLiveRegisters(llvm::IRBuilder<> &builder)
{
	if(builder.GetInsertBlock()->size()) {
		builder.SetInsertPoint(&builder.GetInsertBlock()->back());
	}

	for(auto live_reg : live_register_values_) {
		builder.CreateStore(live_reg.second, live_register_pointers_.at(live_reg.first));
	}
	live_register_values_.clear();
}
