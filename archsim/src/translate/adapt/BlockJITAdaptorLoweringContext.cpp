/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/llvm/LLVMAliasAnalysis.h"
#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "translate/adapt/BlockJITAdaptorLowering.h"
#include "core/thread/ThreadInstance.h"

#include <llvm/IR/Constants.h>

using namespace archsim::translate::adapt;

BlockJITValues::BlockJITValues(::llvm::Module* module)
{
	auto &ctx = module->getContext();
	auto voidTy = llvm::Type::getVoidTy(ctx);
	auto i8PtrTy = llvm::Type::getInt8PtrTy(ctx);
	auto i8PtrPtrTy = llvm::Type::getInt8PtrTy(ctx)->getPointerTo(0);
	
	auto i8Ty = llvm::Type::getInt8Ty(ctx);
	auto i16Ty = llvm::Type::getInt16Ty(ctx);
	auto i32Ty = llvm::Type::getInt32Ty(ctx);
	
	cpuTakeExceptionPtr = (llvm::Function*)module->getOrInsertFunction("cpuTakeException", voidTy, i8PtrTy, i32Ty, i32Ty);
	
	blkRead8Ptr = (llvm::Function*)module->getOrInsertFunction("blkRead8", i8Ty, i8PtrPtrTy, i32Ty, i32Ty);
	blkRead16Ptr = (llvm::Function*)module->getOrInsertFunction("blkRead16", i16Ty, i8PtrPtrTy, i32Ty, i32Ty);
	blkRead32Ptr = (llvm::Function*)module->getOrInsertFunction("blkRead32", i32Ty, i8PtrPtrTy, i32Ty, i32Ty);
	
	blkWrite8Ptr = (llvm::Function*)module->getOrInsertFunction("blkWrite8", voidTy, i8PtrTy, i32Ty, i32Ty, i8Ty);
	blkWrite16Ptr = (llvm::Function*)module->getOrInsertFunction("blkWrite16", voidTy, i8PtrTy, i32Ty, i32Ty, i16Ty);
	blkWrite32Ptr = (llvm::Function*)module->getOrInsertFunction("blkWrite32", voidTy, i8PtrTy, i32Ty, i32Ty, i32Ty);
	
	genc_adc_flags_ptr = (llvm::Function*)module->getOrInsertFunction("genc_adc_flags", i16Ty, i32Ty, i32Ty, i8Ty);
}


bool BlockJITLoweringContext::Prepare(const TranslationContext& ctx) {
	// TODO: add lowerers
	AddLowerer(IRInstruction::ADC_WITH_FLAGS, new BlockJITADCFLAGSLowering());
	AddLowerer(IRInstruction::ADD, new BlockJITADDLowering());
	AddLowerer(IRInstruction::AND, new BlockJITANDLowering());
	AddLowerer(IRInstruction::BARRIER, new BlockJITBARRIERLowering());
	AddLowerer(IRInstruction::BRANCH, new BlockJITBRANCHLowering());
	AddLowerer(IRInstruction::CALL, new BlockJITCALLLowering());
	AddLowerer(IRInstruction::CLZ, new BlockJITCLZLowering());
	AddLowerer(IRInstruction::CMPEQ, new BlockJITCMPLowering(IRInstruction::CMPEQ));
	AddLowerer(IRInstruction::CMPNE, new BlockJITCMPLowering(IRInstruction::CMPNE));
	AddLowerer(IRInstruction::CMPGT, new BlockJITCMPLowering(IRInstruction::CMPGT));
	AddLowerer(IRInstruction::CMPGTE, new BlockJITCMPLowering(IRInstruction::CMPGTE));
	AddLowerer(IRInstruction::CMPLT, new BlockJITCMPLowering(IRInstruction::CMPLT));
	AddLowerer(IRInstruction::CMPLTE, new BlockJITCMPLowering(IRInstruction::CMPLTE));
	AddLowerer(IRInstruction::CMPSGT, new BlockJITCMPLowering(IRInstruction::CMPSGT));
	AddLowerer(IRInstruction::CMPSGTE, new BlockJITCMPLowering(IRInstruction::CMPSGTE));
	AddLowerer(IRInstruction::CMPSLT, new BlockJITCMPLowering(IRInstruction::CMPSLT));
	AddLowerer(IRInstruction::CMPSLTE, new BlockJITCMPLowering(IRInstruction::CMPSLTE));
	AddLowerer(IRInstruction::COUNT, new BlockJITCOUNTLowering());
	AddLowerer(IRInstruction::INCPC, new BlockJITINCPCLowering());
	AddLowerer(IRInstruction::IMUL, new BlockJITUMULLLowering());
	AddLowerer(IRInstruction::JMP, new BlockJITJMPLowering());
	AddLowerer(IRInstruction::LDPC, new BlockJITLDPCLowering());
	AddLowerer(IRInstruction::MOV, new BlockJITMOVLowering());
	AddLowerer(IRInstruction::NOP, new BlockJITNOPLowering());
	AddLowerer(IRInstruction::OR, new BlockJITORLowering());
	AddLowerer(IRInstruction::READ_REG, new BlockJITLDREGLowering());
	AddLowerer(IRInstruction::RET, new BlockJITRETLowering());
	AddLowerer(IRInstruction::SET_CPU_MODE, new BlockJITSCMLowering());
	AddLowerer(IRInstruction::SHL, new BlockJITSHLLowering());
	AddLowerer(IRInstruction::SAR, new BlockJITSARLowering());
	AddLowerer(IRInstruction::SHR, new BlockJITSHRLowering());
	AddLowerer(IRInstruction::SUB, new BlockJITSUBLowering());
	AddLowerer(IRInstruction::SX, new BlockJITMOVSXLowering());
	AddLowerer(IRInstruction::TAKE_EXCEPTION, new BlockJITEXCEPTIONLowering());
	AddLowerer(IRInstruction::TRUNC, new BlockJITTRUNCLowering());
	AddLowerer(IRInstruction::MUL, new BlockJITUMULLLowering());
	AddLowerer(IRInstruction::WRITE_REG, new BlockJITSTREGLowering());
	AddLowerer(IRInstruction::XOR, new BlockJITXORLowering());
	AddLowerer(IRInstruction::SET_ZN_FLAGS, new BlockJITZNFLAGSLowering());
	AddLowerer(IRInstruction::ZX, new BlockJITZXLowering());

	AddLowerer(IRInstruction::READ_MEM, new BlockJITLDMEMUserLowering());
	AddLowerer(IRInstruction::WRITE_MEM, new BlockJITSTMEMUserLowering());
	return true;
}

llvm::BasicBlock* BlockJITLoweringContext::GetLLVMBlock(IRBlockId block_id)
{
	if(!block_ptrs_.count(block_id)) {
		auto block_name = "block_"+std::to_string(block_id);
		auto block = ::llvm::BasicBlock::Create(GetLLVMContext(), block_name, target_fun_);
		block_ptrs_[block_id] = block;
	}
	return block_ptrs_[block_id];
}

::llvm::Type* BlockJITLoweringContext::GetPointerIntType()
{
	// assume 64 bit
	return ::llvm::Type::getInt64Ty(GetLLVMContext());
}

::llvm::Value* BlockJITLoweringContext::GetTaggedRegisterPointer(const std::string& tag)
{
	return GetRegisterPointer(thread_->GetArch().GetRegisterFileDescriptor().GetTaggedEntry(tag), 0);
}

::llvm::Value* BlockJITLoweringContext::GetRegisterPointer(const archsim::RegisterFileEntryDescriptor& reg, int index)
{
	return GetRegisterPointer(reg.GetOffset() + (reg.GetEntryStride()*index), reg.GetEntrySize());
}

::llvm::Value *BlockJITLoweringContext::GetRegisterPointer(int offset, int size)
{
	if(!greg_ptrs_.count({offset, size})) {
		::llvm::IRBuilder<> builder (GetLLVMContext());
		builder.SetInsertPoint(&*target_fun_->begin(), target_fun_->begin()->begin());
		
		auto ptr = GetRegfilePointer();
		ptr = builder.CreatePtrToInt(ptr, GetPointerIntType());
		ptr = builder.CreateAdd(ptr, ::llvm::ConstantInt::get(GetPointerIntType(), offset));
		
		llvm::CastInst *inttoptr = builder.Insert(llvm::CastInst::Create(llvm::CastInst::IntToPtr, ptr, GetLLVMType(size)->getPointerTo(0)));
		inttoptr->setName("reg_" + std::to_string(offset) + "_" + std::to_string(size));
		
		llvm::MDNode *aa_metadata = GetRegAccessMetadata(offset, size);
		
		inttoptr->setMetadata("archsim_aa", aa_metadata);
		
		greg_ptrs_[{offset, size}] = inttoptr;
	}
	
	return greg_ptrs_.at({offset, size});
}

::llvm::Value *BlockJITLoweringContext::GetRegisterPointer(llvm::Value *offset, int size)
{
	auto &builder = GetBuilder();
	
	auto ptr = GetRegfilePointer();
	ptr = builder.CreatePtrToInt(ptr, GetPointerIntType());
	offset = builder.CreateZExtOrTrunc(offset, GetPointerIntType());
	ptr = builder.CreateAdd(ptr, offset);
	ptr = builder.CreateIntToPtr(ptr, GetLLVMType(size)->getPointerTo(0));
	
	return ptr;
}

::llvm::Value* BlockJITLoweringContext::GetRegfilePointer()
{
	// arg 0 is reg file ptr
	auto first_arg = target_fun_->arg_begin();	
	return static_cast<::llvm::Value*>(&*first_arg);
}

::llvm::Value* BlockJITLoweringContext::GetStateBlockPtr()
{
	// arg 1 is reg file ptr
	auto arg = target_fun_->arg_begin();
	arg++;
	return static_cast<::llvm::Value*>(&*arg);
}

::llvm::MDNode* BlockJITLoweringContext::GetRegAccessMetadata(int offset, int size)
{
	return llvm::MDNode::get(GetLLVMContext(), {
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(GetLLVMType(4), archsim::translate::translate_llvm::TAG_REG_ACCESS, 0)),// Tag
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(GetLLVMType(4), offset, 0)),// Offset
		llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(GetLLVMType(4), size, 0))// Size
	});
}

::llvm::MDNode* BlockJITLoweringContext::GetRegAccessMetadata()
{

}


llvm::Value* BlockJITLoweringContext::GetStateBlockEntryPtr(const std::string& entry, llvm::Type *type)
{
	auto offset = GetThread()->GetStateBlock().GetBlockOffset(entry);
	llvm::Value *ptr = GetStateBlockPtr();
	ptr = GetBuilder().CreatePtrToInt(ptr, GetPointerIntType());
	ptr = GetBuilder().CreateAdd(ptr, llvm::ConstantInt::get(GetPointerIntType(), offset, false));
	ptr = GetBuilder().CreateIntToPtr(ptr, type->getPointerTo(0));
	
	return ptr;	
}


llvm::Value* BlockJITLoweringContext::GetThreadPtr()
{
	return GetBuilder().CreateLoad(GetThreadPtrPtr());
}

llvm::Value* BlockJITLoweringContext::GetThreadPtrPtr()
{
	return GetStateBlockEntryPtr("thread_ptr", llvm::Type::getInt8PtrTy(GetLLVMContext()));
}

bool BlockJITLoweringContext::LowerBlock(const TranslationContext& ctx, captive::shared::IRBlockId block_id, uint32_t block_start) {	
	auto llvm_block = GetLLVMBlock(block_id);
	builder_->SetInsertPoint(llvm_block);
	
	const IRInstruction *insn = ctx.at(block_start);
	while (insn < ctx.end() && (insn->ir_block == NOP_BLOCK || insn->ir_block == block_id)) {
		if (!LowerInstruction(ctx, insn)) {
			//				LC_ERROR(LogLower) << "Could not lower IRInstruction";
			return false;
		}
	}

	return true;
}

llvm::Value* BlockJITLoweringContext::GetValueFor(const IROperand& operand) {
	switch(operand.type) {
		case IROperand::FUNC:
		case IROperand::CONSTANT:
			return ::llvm::ConstantInt::get(GetLLVMType(operand), operand.value);
		case IROperand::VREG:
			return builder_->CreateLoad(GetRegPtr(operand));
		default:
			UNIMPLEMENTED;
	}
}

void BlockJITLoweringContext::SetValueFor(const IROperand& operand, ::llvm::Value* value) {
	auto ptr = GetRegPtr(operand);
	builder_->CreateStore(value, ptr);
}

::llvm::Type* BlockJITLoweringContext::GetLLVMType(uint32_t bytes) {
	switch(bytes) {
		case 1: return ::llvm::Type::getInt8Ty(GetLLVMContext());
		case 2: return ::llvm::Type::getInt16Ty(GetLLVMContext());
		case 4: return ::llvm::Type::getInt32Ty(GetLLVMContext());
		case 8: return ::llvm::Type::getInt64Ty(GetLLVMContext());
		default:
			UNIMPLEMENTED;
	}
}

::llvm::Type* BlockJITLoweringContext::GetLLVMType(const IROperand& op)
{
	if(op.type == IROperand::FUNC) {
		return ::llvm::Type::getInt64Ty(GetLLVMContext());
	}
	return GetLLVMType(op.size);
}

::llvm::Value* BlockJITLoweringContext::GetRegPtr(const IROperand &op)
{
	assert(op.is_vreg());
	
	if(!reg_ptrs_.count(op.get_vreg_idx())) {
		auto prev_bb = builder_->GetInsertBlock();
		auto prev_ip = builder_->GetInsertPoint();
		
		builder_->SetInsertPoint(&target_fun_->getEntryBlock(), target_fun_->getEntryBlock().begin());
		auto ptr = builder_->CreateAlloca(GetLLVMType(op), 0, nullptr);
		
		builder_->SetInsertPoint(prev_bb, prev_ip);
		
		reg_ptrs_[op.get_vreg_idx()] = ptr;
	}
	return reg_ptrs_.at(op.get_vreg_idx());
}
