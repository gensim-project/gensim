/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMGuestRegisterAccessEmitter.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMAliasAnalysis.h"

#include <llvm/IR/DerivedTypes.h>

using namespace archsim::translate::translate_llvm;

LLVMGuestRegisterAccessEmitter::LLVMGuestRegisterAccessEmitter(LLVMTranslationContext& ctx) : ctx_(ctx)
{

}

LLVMGuestRegisterAccessEmitter::~LLVMGuestRegisterAccessEmitter()
{

}

void LLVMGuestRegisterAccessEmitter::AddAAAIMetadata(llvm::Value* target, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	if(llvm::isa<llvm::Instruction>(target)) {
		llvm::Instruction *insn = llvm::dyn_cast<llvm::Instruction>(target);

		insn->setMetadata("aaai", GetMetadataFor(reg, index));
	} else if(llvm::isa<llvm::ConstantExpr>(target)) {
//		llvm::ConstantExpr *insn = llvm::dyn_cast<llvm::ConstantExpr>(target);
//		insn->setMetadata("aaai", GetMetadataFor(reg, index));
	} else {
		// coukldn't figure out what AAAI to add
	}
}

llvm::MDNode *LLVMGuestRegisterAccessEmitter::GetMetadataFor(const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index)
{
	std::vector<int> output {archsim::translate::translate_llvm::TAG_REG_ACCESS};

	std::vector<llvm::Metadata*> mddata;
	for(auto i : output) {
		mddata.push_back(llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(GetCtx().Types.i64, i)));
	}
	return llvm::MDNode::get(GetCtx().LLVMCtx, mddata);
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
	return builder.CreateLoad(GetPointerToReg(builder, reg, index));
}

void GEPLLVMGuestRegisterAccessEmitter::EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value)
{
	builder.CreateStore(value, GetPointerToReg(builder, reg, index));
}

llvm::Value* GEPLLVMGuestRegisterAccessEmitter::GetPointerToReg(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg_view, llvm::Value* index)
{
	if(index == nullptr) {
		index = llvm::ConstantInt::get(GetCtx().Types.i64, 0);
	}

	llvm::Value *ptr = nullptr;
	llvm::IRBuilder<> ptr_builder = builder;

	if(llvm::isa<llvm::ConstantInt>(index)) {
		ptr_builder.SetInsertPoint(&builder.GetInsertBlock()->getParent()->getEntryBlock(), builder.GetInsertBlock()->getParent()->getEntryBlock().begin());

		llvm::ConstantInt *c = (llvm::ConstantInt*)index;
		if(c->getZExtValue() >= reg_view.GetRegisterCount()) {
			throw std::logic_error("Register access out of bounds!");
		}

		if(!register_pointer_cache_.count({reg_view.GetName(), c->getZExtValue()})) {
			ptr = GetPointerToRegBank(ptr_builder, reg_view);
			ptr = ptr_builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, 0), index, llvm::ConstantInt::get(GetCtx().Types.i32, 0)});
			register_pointer_cache_[ {reg_view.GetName(), c->getZExtValue()}] = ptr;
		} else {
			ptr = register_pointer_cache_[ {reg_view.GetName(), c->getZExtValue()}];
		}
	} else {
		// Emit range check if debug is enabled
		if(archsim::options::Debug) {
			llvm::BasicBlock *range_ok = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());
			llvm::BasicBlock *range_bad = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

			auto in_range = ptr_builder.CreateICmpULT(index, llvm::ConstantInt::get(index->getType(), reg_view.GetRegisterCount()));
			ptr_builder.CreateCondBr(in_range, range_ok, range_bad);

			ptr_builder.SetInsertPoint(range_bad);
			ptr_builder.CreateCall(GetCtx().Functions.debug_trap);
			ptr_builder.CreateRet(llvm::ConstantInt::get(GetCtx().Types.i32, 1));

			ptr_builder.SetInsertPoint(range_ok);
		}

		ptr = GetPointerToRegBank(ptr_builder, reg_view);
		ptr = ptr_builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, 0), index, llvm::ConstantInt::get(GetCtx().Types.i32, 0)});
	}

	AddAAAIMetadata(ptr, reg_view, index);

	return ptr;
}

llvm::Value* GEPLLVMGuestRegisterAccessEmitter::GetPointerToRegBank(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg_view)
{
	llvm::Value *ptr = GetCtx().Values.reg_file_ptr;
	ptr = builder.CreateInBoundsGEP(ptr, {llvm::ConstantInt::get(GetCtx().Types.i64, reg_view.GetOffset())});
	ptr = builder.CreatePointerCast(ptr, GetTypeForRegView(reg_view)->getPointerTo(0));
	AddAAAIMetadata(ptr, reg_view, nullptr);
	return ptr;
}

llvm::Type* GEPLLVMGuestRegisterAccessEmitter::GetTypeForRegView(const archsim::RegisterFileEntryDescriptor& reg_view)
{
	return llvm::ArrayType::get(GetTypeForRegViewEntry(reg_view), reg_view.GetRegisterCount());
}

llvm::Type* GEPLLVMGuestRegisterAccessEmitter::GetTypeForRegViewEntry(const archsim::RegisterFileEntryDescriptor& reg_view)
{
	llvm::Type *base_type = nullptr;
	if(reg_view.GetEntryCountPerRegister() > 1) {
		// vector register
		base_type = llvm::IntegerType::get(GetCtx().LLVMCtx, reg_view.GetEntrySize()*8);
		base_type = llvm::VectorType::get(base_type, reg_view.GetEntryCountPerRegister());
	} else {
		// scalar register
		base_type = llvm::IntegerType::get(GetCtx().LLVMCtx, reg_view.GetEntrySize()*8);
	}

	assert(reg_view.GetRegisterStride() >= reg_view.GetRegisterSize());
	int padding_bytes = reg_view.GetRegisterStride() - reg_view.GetRegisterSize();

	assert(padding_bytes >= 0);

	std::vector<llvm::Type*> entries;
	entries.push_back(base_type);

	if(padding_bytes) {
		llvm::ArrayType *padding_array = llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(GetCtx().LLVMCtx), padding_bytes);
		entries.push_back(padding_array);
	}

	return llvm::StructType::get(GetCtx().LLVMCtx, entries);
}
