/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMGuestRegisterAccessEmitter.h"
#include "translate/llvm/LLVMTranslationContext.h"

#include <llvm/IR/DerivedTypes.h>

using namespace archsim::translate::tx_llvm;

LLVMGuestRegisterAccessEmitter::LLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : ctx_(ctx)
{

}

LLVMGuestRegisterAccessEmitter::~LLVMGuestRegisterAccessEmitter()
{

}

BasicLLVMGuestRegisterAccessEmitter::BasicLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : LLVMGuestRegisterAccessEmitter(ctx)
{

}

BasicLLVMGuestRegisterAccessEmitter::~BasicLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* BasicLLVMGuestRegisterAccessEmitter::EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	auto ptr = GetRegisterPointer(builder, reg, index);
	return builder.CreateLoad(ptr, "register_entry_from_" + reg.GetName());
}

void BasicLLVMGuestRegisterAccessEmitter::EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	auto ptr = GetRegisterPointer(builder, reg, index);
	ptr = builder.CreatePointerCast(ptr, value->getType()->getPointerTo(0));
	builder.CreateStore(value, ptr);
}

llvm::Value* BasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	auto ptr = GetCtx().Values.reg_file_ptr;
	ptr = builder.CreatePtrToInt(ptr, GetCtx().Types.i64);
	if(reg.GetOffset() != 0) {
		ptr = builder.CreateAdd(ptr, llvm::ConstantInt::get(GetCtx().Types.i64, reg.GetOffset()), "ptr_to_" + reg.GetName());
	}
	if(index != nullptr) {
		index = builder.CreateZExt(index, GetCtx().Types.i64);
		ptr = builder.CreateAdd(ptr, builder.CreateMul(index, llvm::ConstantInt::get(GetCtx().Types.i64, reg.GetRegisterStride())), "ptr_to_" + reg.GetName() + "_entry");
	}
	ptr = builder.CreateIntToPtr(ptr, llvm::IntegerType::getIntNPtrTy(GetCtx().LLVMCtx, reg.GetRegisterSize()*8));
	return ptr;
}

CachingBasicLLVMGuestRegisterAccessEmitter::CachingBasicLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : BasicLLVMGuestRegisterAccessEmitter(ctx)
{

}

CachingBasicLLVMGuestRegisterAccessEmitter::~CachingBasicLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* CachingBasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	bool index_is_constant = false;
	uint64_t constant_index = 0;
	if(index == nullptr) {
		index_is_constant = true;
	} else if(llvm::isa<llvm::ConstantInt>(index)) {
		constant_index = llvm::dyn_cast<llvm::ConstantInt>(index)->getZExtValue();
		index_is_constant = true;
	}

	if(index_is_constant) {
		if(!register_pointer_cache_.count({reg.GetName(), constant_index})) {
			llvm::IRBuilder<> entry_builder (GetCtx().LLVMCtx);
			entry_builder.SetInsertPoint(&builder.GetInsertBlock()->getParent()->getEntryBlock(), builder.GetInsertBlock()->getParent()->getEntryBlock().begin());

			register_pointer_cache_[ {reg.GetName(), constant_index}] = BasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(entry_builder, reg, index);
		}

		return register_pointer_cache_.at({reg.GetName(), constant_index});
	} else {
		register_pointer_cache_.clear();
		return BasicLLVMGuestRegisterAccessEmitter::GetRegisterPointer(builder, reg, index);
	}
}


GEPLLVMGuestRegisterAccessEmitter::GEPLLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : LLVMGuestRegisterAccessEmitter(ctx)
{

}

GEPLLVMGuestRegisterAccessEmitter::~GEPLLVMGuestRegisterAccessEmitter()
{

}

llvm::Value* GEPLLVMGuestRegisterAccessEmitter::EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	UNIMPLEMENTED;
	return nullptr;
}

void GEPLLVMGuestRegisterAccessEmitter::EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	UNIMPLEMENTED;
}

llvm::Type* GEPLLVMGuestRegisterAccessEmitter::GetTypeForRegView(const archsim::RegisterFileEntryDescriptor& reg_view)
{
	return llvm::ArrayType::get(GetTypeForRegViewEntry(reg_view), reg_view.GetEntryCountPerRegister());
}

llvm::Type* GEPLLVMGuestRegisterAccessEmitter::GetTypeForRegViewEntry(const archsim::RegisterFileEntryDescriptor& reg_view)
{
	llvm::IntegerType *base_type = llvm::IntegerType::get(GetCtx().LLVMCtx, reg_view.GetEntrySize()*8);
	int padding_bytes = reg_view.GetEntryStride() - reg_view.GetEntrySize();

	std::vector<llvm::Type*> entries;
	entries.push_back(base_type);

	if(padding_bytes) {
		llvm::ArrayType *padding_array = llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(GetCtx().LLVMCtx), padding_bytes);
		entries.push_back(padding_array);
	}

	return llvm::StructType::get(GetCtx().LLVMCtx, entries);
}
