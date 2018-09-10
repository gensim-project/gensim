/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMTranslationContext.h"

using namespace archsim::translate::tx_llvm;

LLVMTranslationContext::LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::Module *module, archsim::core::thread::ThreadInstance *thread) : LLVMCtx(ctx), thread_(thread), Module(module)
{
	Values.reg_file_ptr = nullptr;
	Values.state_block_ptr = nullptr;

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

	Functions.ctpop_i32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::ctpop, Types.i32);
	Functions.bswap_i32 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::bswap, Types.i32);
	Functions.bswap_i64 = llvm::Intrinsic::getDeclaration(Module, llvm::Intrinsic::bswap, Types.i64);

	Functions.jit_trap =  (llvm::Function*)Module->getOrInsertFunction("cpuTrap", Types.vtype, Types.i8Ptr);

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

}

llvm::Value* LLVMTranslationContext::GetThreadPtr()
{
	auto ptr = Values.state_block_ptr;
	return GetBuilder().CreateLoad(GetBuilder().CreatePointerCast(ptr, Types.i8Ptr->getPointerTo(0)));
}

llvm::Value* LLVMTranslationContext::GetRegStatePtr()
{
	return Values.reg_file_ptr;
}

llvm::Value* LLVMTranslationContext::GetStateBlockPointer(const std::string& entry)
{
	auto ptr = Values.state_block_ptr;
	ptr = GetBuilder().CreatePtrToInt(ptr, Types.i64);
	ptr = GetBuilder().CreateAdd(ptr, llvm::ConstantInt::get(Types.i64, thread_->GetStateBlock().GetBlockOffset(entry)));
	ptr = GetBuilder().CreateIntToPtr(ptr, Types.i8Ptr);
	return ptr;
}

llvm::Value* LLVMTranslationContext::AllocateRegister(llvm::Type *type, int name)
{
	if(free_registers_[type].empty()) {
		auto &block = GetBuilder().GetInsertBlock()->getParent()->getEntryBlock();

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

void LLVMTranslationContext::SetBuilder(llvm::IRBuilder<>& builder)
{
	builder_ = &builder;

	Module = GetBuilder().GetInsertBlock()->getParent()->getParent();
	Values.reg_file_ptr =    GetBuilder().GetInsertBlock()->getParent()->arg_begin();
	Values.state_block_ptr = GetBuilder().GetInsertBlock()->getParent()->arg_begin() + 1;
}

llvm::Value* LLVMTranslationContext::GetRegPtr(int offset, llvm::Type* type)
{
	if(guest_reg_ptrs_.count({offset, type}) == 0) {

		// insert pointer calculation into header block
		// ptrtoint
		auto &insert_point = GetBuilder().GetInsertBlock()->getParent()->getEntryBlock().front();

		llvm::IRBuilder<> sub_builder(&insert_point);
		llvm::Value *ptr = sub_builder.CreatePtrToInt(GetRegStatePtr(), Types.i64);
		ptr = sub_builder.CreateAdd(ptr, llvm::ConstantInt::get(Types.i64, offset));
		ptr = sub_builder.CreateIntToPtr(ptr, type->getPointerTo(0));

		guest_reg_ptrs_[ {offset, type}] = ptr;
	}

	return guest_reg_ptrs_.at({offset, type});
}

llvm::Value* LLVMTranslationContext::LoadRegister(int name)
{
	if(!live_register_values_.count(name)) {
		live_register_values_[name] = GetBuilder().CreateLoad(live_register_pointers_.at(name));
	}
	return live_register_values_.at(name);
}

void LLVMTranslationContext::StoreRegister(int name, llvm::Value* value)
{
	live_register_values_[name] = value;
}

void LLVMTranslationContext::SetBlock(llvm::BasicBlock* block)
{
//	ResetLiveRegisters();
	GetBuilder().SetInsertPoint(block);
}

void LLVMTranslationContext::ResetLiveRegisters()
{
	// insert just before the block terminators
	if(GetBuilder().GetInsertBlock()->size()) {
		GetBuilder().SetInsertPoint(&GetBuilder().GetInsertBlock()->back());
	}

	for(auto live_reg : live_register_values_) {
		GetBuilder().CreateStore(live_reg.second, live_register_pointers_.at(live_reg.first));
	}
	live_register_values_.clear();
}
