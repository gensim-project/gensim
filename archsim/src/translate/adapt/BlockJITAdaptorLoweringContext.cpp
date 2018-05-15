/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLoweringContext.h"
#include "translate/adapt/BlockJITAdaptorLowering.h"

#include <llvm/IR/Constants.h>

using namespace archsim::translate::adapt;

BlockJITValues::BlockJITValues(llvm::Module* module)
{
	auto &ctx = module->getContext();
	auto voidTy = llvm::Type::getVoidTy(ctx);
	auto i8PtrTy = llvm::Type::getInt8PtrTy(ctx);
	auto i32Ty = llvm::Type::getInt32Ty(ctx);
	
	llvm::GlobalValue *cpuTakeExceptionGV = (llvm::GlobalValue*)module->getOrInsertFunction("cpuTakeException", voidTy, i8PtrTy, i32Ty, i32Ty);
	cpuTakeExceptionGV->setLinkage(llvm::Function::ExternalLinkage);
	cpuTakeExceptionPtr = (llvm::Function*)cpuTakeExceptionGV;
}


bool BlockJITLoweringContext::Prepare(const TranslationContext& ctx) {
	// TODO: add lowerers
	AddLowerer(IRInstruction::ADD, new BlockJITADDLowering());
	AddLowerer(IRInstruction::AND, new BlockJITANDLowering());
	AddLowerer(IRInstruction::BARRIER, new BlockJITBARRIERLowering());
	AddLowerer(IRInstruction::INCPC, new BlockJITINCPCLowering());
	AddLowerer(IRInstruction::JMP, new BlockJITJMPLowering());
	AddLowerer(IRInstruction::MOV, new BlockJITMOVLowering());
	AddLowerer(IRInstruction::LDPC, new BlockJITLDPCLowering());
	AddLowerer(IRInstruction::READ_REG, new BlockJITLDREGLowering());
	AddLowerer(IRInstruction::READ_MEM, new BlockJITLDMEMLowering());
	AddLowerer(IRInstruction::RET, new BlockJITRETLowering());
	AddLowerer(IRInstruction::TAKE_EXCEPTION, new BlockJITEXCEPTIONLowering());
	AddLowerer(IRInstruction::TRUNC, new BlockJITTRUNCLowering());
	AddLowerer(IRInstruction::WRITE_REG, new BlockJITSTREGLowering());
	AddLowerer(IRInstruction::WRITE_MEM, new BlockJITSTMEMLowering());
	AddLowerer(IRInstruction::ZX, new BlockJITZXLowering());

	return true;
}

llvm::BasicBlock* BlockJITLoweringContext::GetLLVMBlock(IRBlockId block_id)
{
	if(!block_ptrs_.count(block_id)) {
		auto block_name = "block_"+std::to_string(block_id);
		auto block = llvm::BasicBlock::Create(GetLLVMContext(), block_name, target_fun_);
		block_ptrs_[block_id] = block;
	}
	return block_ptrs_[block_id];
}

::llvm::Type* BlockJITLoweringContext::GetPointerIntType()
{
	// assume 64 bit
	return ::llvm::Type::getInt64Ty(GetLLVMContext());
}

::llvm::Value* BlockJITLoweringContext::GetRegisterPointer(const archsim::RegisterFileEntryDescriptor& reg, int index)
{
	auto ptr = GetRegfilePointer();
	ptr = GetBuilder().CreatePtrToInt(ptr, GetPointerIntType());
	ptr = GetBuilder().CreateAdd(ptr, ::llvm::ConstantInt::get(GetPointerIntType(), reg.GetOffset() + (reg.GetEntryStride() * index)));
	ptr = GetBuilder().CreateIntToPtr(ptr, GetLLVMType(reg.GetEntrySize())->getPointerTo(0));
	return ptr;
}

::llvm::Value *BlockJITLoweringContext::GetRegisterPointer(int offset, int size)
{
	auto ptr = GetRegfilePointer();
	
	ptr = GetBuilder().CreatePtrToInt(ptr, GetPointerIntType());
	ptr = GetBuilder().CreateAdd(ptr, ::llvm::ConstantInt::get(GetPointerIntType(), offset));
	ptr = GetBuilder().CreateIntToPtr(ptr, GetLLVMType(size)->getPointerTo(0));
	
	return ptr;
}

::llvm::Value* BlockJITLoweringContext::GetRegfilePointer()
{
	// arg 0 is reg file ptr
	auto first_arg = target_fun_->arg_begin();	
	return static_cast<::llvm::Value*>(&*first_arg);
}


llvm::Value* BlockJITLoweringContext::GetThreadPtr()
{
	// arg 1 is state block ptr
	auto arg = target_fun_->arg_begin();
	arg++;
	llvm::Value *control_block_ptr = static_cast<llvm::Value*>(&*arg);
	
	llvm::Value *thread_ptr_ptr = GetBuilder().CreateBitOrPointerCast(control_block_ptr, GetLLVMType(1)->getPointerTo(0)->getPointerTo(0));
	return GetBuilder().CreateLoad(thread_ptr_ptr);
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
		case IROperand::CONSTANT:
			return ::llvm::ConstantInt::get(GetLLVMType(operand), operand.value);
		case IROperand::VREG:
			return builder_->CreateLoad(GetRegPtr(operand));
		default:
			UNIMPLEMENTED;
	}
}

void BlockJITLoweringContext::SetValueFor(const IROperand& operand, llvm::Value* value) {
	auto ptr = GetRegPtr(operand);
	builder_->CreateStore(value, ptr);
}

llvm::Type* BlockJITLoweringContext::GetLLVMType(uint32_t bytes) {
	switch(bytes) {
		case 1: return ::llvm::Type::getInt8Ty(GetLLVMContext());
		case 2: return ::llvm::Type::getInt16Ty(GetLLVMContext());
		case 4: return ::llvm::Type::getInt32Ty(GetLLVMContext());
		case 8: return ::llvm::Type::getInt64Ty(GetLLVMContext());
		default:
			UNIMPLEMENTED;
	}
}

llvm::Type* BlockJITLoweringContext::GetLLVMType(const IROperand& op)
{
	return GetLLVMType(op.size);
}

llvm::Value* BlockJITLoweringContext::GetRegPtr(const IROperand &op)
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
