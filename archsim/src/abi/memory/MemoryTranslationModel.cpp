#include "abi/memory/MemoryTranslationModel.h"
#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "translate/TranslationContext.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "gensim/gensim_processor.h"
#include "util/SimOptions.h"
#include "util/ComponentManager.h"
#include "abi/devices/MMU.h"

#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Instruction.def>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>

using namespace archsim::abi::memory;

MemoryTranslationModel::MemoryTranslationModel()
{
}

MemoryTranslationModel::~MemoryTranslationModel()
{
}

bool MemoryTranslationModel::EmitNonPrivilegedRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	llvm::IRBuilder<>& builder = insn_ctx.block.region.builder;
	llvm::Function *read_fn;
	llvm::Value *read_temp;

	// Select the correct memory read function
	switch (width) {
		case 1:
			read_fn = insn_ctx.block.region.txln.jit_functions.cpu_read_8_user;
			read_temp = insn_ctx.block.region.values.mem_read_temp_8;
			break;
		case 4:
			read_fn = insn_ctx.block.region.txln.jit_functions.cpu_read_32_user;
			read_temp = insn_ctx.block.region.values.mem_read_temp_32;
			break;
		default:
			assert(false);
			return false;
	}

	// Call the memory read function.
	llvm::CallInst *mem_call = builder.CreateCall3(read_fn, insn_ctx.block.region.values.cpu_ctx_val, address, read_temp, "mem_read_result");
	fault = mem_call;

	llvm::BasicBlock *cont_block = llvm::BasicBlock::Create(insn_ctx.block.region.txln.llvm_ctx, "mem_write_cont", insn_ctx.block.region.region_fn);
	llvm::BasicBlock *bail_block = llvm::BasicBlock::Create(insn_ctx.block.region.txln.llvm_ctx, "mem_write_bail", insn_ctx.block.region.region_fn);

	llvm::Value *cond = builder.CreateICmpNE(fault, llvm::ConstantInt::get(insn_ctx.block.region.txln.types.i32, (uint64_t)0));
	builder.CreateCondBr(cond, bail_block, cont_block);

	builder.SetInsertPoint(bail_block);

	insn_ctx.block.region.EmitLeaveInstruction(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, insn_ctx.tiu.GetOffset());

	builder.SetInsertPoint(cont_block);

	// Sign-extend or zero-extend the result to the destination type.
	llvm::Value *data;
	if (sx) {
		data = builder.CreateCast(llvm::Instruction::SExt, builder.CreateLoad(read_temp), destinationType);
	} else {
		data = builder.CreateCast(llvm::Instruction::ZExt, builder.CreateLoad(read_temp), destinationType);
	}

	// Store result back in destination.
	builder.CreateStore(data, destination);

	return true;
}

bool MemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	llvm::IRBuilder<>& builder = insn_ctx.block.region.builder;
	llvm::Function *read_fn;
	llvm::Value *read_temp;

	// Select the correct memory read function
	switch (width) {
		case 1:
			read_fn = insn_ctx.block.region.txln.jit_functions.cpu_read_8;
			read_temp = insn_ctx.block.region.values.mem_read_temp_8;
			break;
		case 2:
			read_fn = insn_ctx.block.region.txln.jit_functions.cpu_read_16;
			read_temp = insn_ctx.block.region.values.mem_read_temp_16;
			break;
		case 4:
			read_fn = insn_ctx.block.region.txln.jit_functions.cpu_read_32;
			read_temp = insn_ctx.block.region.values.mem_read_temp_32;
			break;
		default:
			assert(false);
			return false;
	}

	// Call the memory read function.
	llvm::CallInst *mem_call = builder.CreateCall3(read_fn, insn_ctx.block.region.values.cpu_ctx_val, address, read_temp, "mem_read_result");
	fault = mem_call;

	// Sign-extend or zero-extend the result to the destination type.
	llvm::Value *data;
	if (sx) {
		data = builder.CreateCast(llvm::Instruction::SExt, builder.CreateLoad(read_temp), destinationType);
	} else {
		data = builder.CreateCast(llvm::Instruction::ZExt, builder.CreateLoad(read_temp), destinationType);
	}

	// Store result back in destination.
	builder.CreateStore(data, destination);

	return true;
}

bool MemoryTranslationModel::EmitNonPrivilegedWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	llvm::IRBuilder<>& builder = insn_ctx.block.region.builder;
	llvm::Function *write_fn = NULL;
	llvm::Type *write_data_type;

	// Depending on the write instruction width, select the
	// correct memory function.
	switch (width) {
		case 1:
			write_fn = insn_ctx.block.region.txln.jit_functions.cpu_write_8_user;
			write_data_type = insn_ctx.block.region.txln.types.i8;
			break;
		case 4:
			write_fn = insn_ctx.block.region.txln.jit_functions.cpu_write_32_user;
			write_data_type = insn_ctx.block.region.txln.types.i32;
			break;
		default:
			assert(false);
			return false;
	}

	// Cast the incoming value to be stored to an i32, which is
	// accepted by the write function.
	if (value->getType() != write_data_type)
		value = builder.CreateCast(llvm::Instruction::Trunc, value, write_data_type);

	// Call the write function.
	fault = builder.CreateCall3(write_fn, insn_ctx.block.region.values.cpu_ctx_val, address, value, "mem_write_result");

	return true;
}

bool MemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	llvm::IRBuilder<>& builder = insn_ctx.block.region.builder;
	llvm::Function *write_fn;
	llvm::Type *write_data_type;

	// Depending on the write instruction width, select the
	// correct memory function.
	switch (width) {
		case 1:
			write_fn = insn_ctx.block.region.txln.jit_functions.cpu_write_8;
			write_data_type = insn_ctx.block.region.txln.types.i8;
			break;
		case 2:
			write_fn = insn_ctx.block.region.txln.jit_functions.cpu_write_16;
			write_data_type = insn_ctx.block.region.txln.types.i16;
			break;
		case 4:
			write_fn = insn_ctx.block.region.txln.jit_functions.cpu_write_32;
			write_data_type = insn_ctx.block.region.txln.types.i32;
			break;
		default:
			return false;
	}

	// Cast the incoming value to be stored to an i32, which is
	// accepted by the write function.
	if (value->getType() != write_data_type)
		value = builder.CreateCast(llvm::Instruction::Trunc, value, write_data_type);

	// Call the write function.
	fault = builder.CreateCall3(write_fn, insn_ctx.block.region.values.cpu_ctx_val, address, value, "mem_write_result");

	// if (fault == TXLN_INTERNAL_BEAN) bail_out

	llvm::BasicBlock *cont_block = llvm::BasicBlock::Create(insn_ctx.block.region.txln.llvm_ctx, "mem_write_cont", insn_ctx.block.region.region_fn);
	llvm::BasicBlock *bail_block = llvm::BasicBlock::Create(insn_ctx.block.region.txln.llvm_ctx, "mem_write_bail", insn_ctx.block.region.region_fn);

	llvm::Value *cond = builder.CreateICmpUGE(fault, llvm::ConstantInt::get(insn_ctx.block.region.txln.types.i32, (uint64_t)archsim::abi::devices::MMU::TXLN_INTERNAL_BASE));
	builder.CreateCondBr(cond, bail_block, cont_block);

	builder.SetInsertPoint(bail_block);

	insn_ctx.block.region.builder.CreateCall(insn_ctx.block.region.txln.jit_functions.cpu_return_to_safepoint, insn_ctx.block.region.values.cpu_ctx_val);//->setDoesNotReturn();
	insn_ctx.block.region.EmitLeaveInstruction(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, insn_ctx.tiu.GetOffset());

	builder.SetInsertPoint(cont_block);
	return true;
}

bool MemoryTranslationModel::EmitPerformTranslation(archsim::translate::llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *virt_address, llvm::Value *&phys_address, llvm::Value *&fault)
{
	phys_address = virt_address;
	fault = llvm::ConstantInt::get(insn_ctx.txln.types.i32, 0, false);
	return true;
}

bool MemoryTranslationModel::PrepareTranslation(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx)
{
	return true;
}

ContiguousMemoryTranslationModel::ContiguousMemoryTranslationModel()
{
}

ContiguousMemoryTranslationModel::~ContiguousMemoryTranslationModel()
{
}

bool ContiguousMemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventRead, address, (uint8_t)width);
	}

	fault = llvm::ConstantInt::get(insn_ctx.block.region.txln.types.i32, 0);
	address = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, address, insn_ctx.block.region.txln.types.i64);

	llvm::Value* real_addr = insn_ctx.block.region.builder.CreateAdd(address, llvm::ConstantInt::get(insn_ctx.block.region.txln.types.i64, (uint64_t) contiguousMemoryBase, 0));

	switch (width) {
		case 1:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi8);
			break;
		case 2:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi16);
			break;
		case 4:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi32);
			break;
		case 8:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi64);
			break;
		default:
			return false;
	}

	if (real_addr->getValueID() >= llvm::Value::InstructionVal)
		insn_ctx.AddAliasAnalysisNode((llvm::Instruction*)real_addr, archsim::translate::llvm::TAG_MEM_ACCESS);

	llvm::Value* value = insn_ctx.block.region.builder.CreateLoad(real_addr);
	if (sx) {
		value = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::SExt, value, destinationType);
	} else {
		value = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, destinationType);
	}

	insn_ctx.block.region.builder.CreateStore(value, destination);
	return true;
}

bool ContiguousMemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventWrite, address, (uint8_t)width);
	}

	fault = llvm::ConstantInt::get(insn_ctx.block.region.txln.types.i32, 0);

	address = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, address, insn_ctx.block.region.txln.types.i64);

	llvm::Value* real_addr = insn_ctx.block.region.builder.CreateAdd(address, llvm::ConstantInt::get(insn_ctx.block.region.txln.types.i64, (uint64_t) contiguousMemoryBase, 0));
	switch (width) {
		case 1:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi8);
			value = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, insn_ctx.block.region.txln.types.i8);
			break;
		case 2:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi16);
			value = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, insn_ctx.block.region.txln.types.i16);
			break;
		case 4:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi32);
			value = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, insn_ctx.block.region.txln.types.i32);
			break;
		case 8:
			real_addr = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::IntToPtr, real_addr, insn_ctx.block.region.txln.types.pi64);
			value = insn_ctx.block.region.builder.CreateCast(llvm::Instruction::ZExt, value, insn_ctx.block.region.txln.types.i64);
			break;
	}

	if (real_addr->getValueID() >= llvm::Value::InstructionVal)
		insn_ctx.AddAliasAnalysisNode((llvm::Instruction*)real_addr, archsim::translate::llvm::TAG_MEM_ACCESS);
	insn_ctx.block.region.builder.CreateStore(value, real_addr);

	return true;
}
