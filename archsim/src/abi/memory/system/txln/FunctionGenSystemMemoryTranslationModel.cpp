/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * FunctionGenSystemMemoryTranslationModel.cpp
 *
 *  Created on: 9 Sep 2014
 *      Author: harry
 */

#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "abi/memory/system/FunctionBasedSystemMemoryModel.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/MMU.h"
#include "gensim/gensim_processor.h"
#include "translate/TranslationWorkUnit.h"
#include "ij/arch/x86/macros.h"

using namespace archsim::abi::memory;

UseLogContext(LogSystemMemoryModel);
UseLogContext(LogFunctionSystemMemoryModel);

FunctionGenSystemMemoryTranslationModel::FunctionGenSystemMemoryTranslationModel() : dirty(true),Complete(0),mem_model(NULL),subscriber(NULL),write_cache(NULL),read_cache(NULL), write_cache_size(0), read_cache_size(0), write_user_cache(NULL), write_user_cache_size(0), read_user_cache(NULL), read_user_cache_size(0)
{

}

static bool HandleSegFaultRead(void *ctx, const System::segfault_data& data)
{
	uint64_t *page_base = (uint64_t *)(data.addr & ~4095ULL);
	mprotect(page_base, 4096, PROT_READ | PROT_WRITE);

	for (int i = 0; i < 512; i++)
		page_base[i] = (uint64_t)DefaultReadHandlerShunt;

	return true;
}

static bool HandleSegFaultWrite(void *ctx, const System::segfault_data& data)
{
	uint64_t *page_base = (uint64_t *)(data.addr & ~4095ULL);
	mprotect(page_base, 4096, PROT_READ | PROT_WRITE);

	for (int i = 0; i < 512; i++)
		page_base[i] = (uint64_t)DefaultWriteHandlerSave;

	return true;
}

static void TlbCallback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	auto fgen = (FunctionGenSystemMemoryTranslationModel*)ctx;

	switch(type) {
		case PubSubType::ITlbEntryFlush:
		case PubSubType::DTlbEntryFlush:
		case PubSubType::RegionDispatchedForTranslationVirtual: {
			virt_addr_t addr = (virt_addr_t) (uint64_t)data;
			fgen->Evict(addr);
			break;
		}
		case PubSubType::ITlbFullFlush:
		case PubSubType::DTlbFullFlush:
//	case PubSubType::RegionDispatchedForTranslationPhysical:
			fgen->Flush();
			break;
		default:
			assert(false);
	}

}

static void TimerCallback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	FunctionGenSystemMemoryTranslationModel *model = (FunctionGenSystemMemoryTranslationModel*)ctx;
	switch(type) {
		case PubSubType::UserEvent1:
			//clear histogram
			model->fn_histogram.clear();
			return;
		case PubSubType::UserEvent2:
			//print histogram
			std::cerr << "XXX Function histogram start" << std::endl;
			std::cerr << model->fn_histogram.to_string();
			std::cerr << "XXX Function histogram end" << std::endl;
			return;
		default:
			assert(false);
	}
}

bool FunctionGenSystemMemoryTranslationModel::Initialise(SystemMemoryModel &smem_model)
{
	Complete = true;
	this->mem_model = (FunctionBasedSystemMemoryModel*)&smem_model;

	int mmap_prot = PROT_READ | PROT_WRITE;

	if (archsim::options::LazyMemoryModelInvalidation) {
		mmap_prot = PROT_NONE;
	}

	read_cache_size = sizeof(read_handler_t) * archsim::translate::profile::RegionArch::PageCount;
	read_cache = (read_handler_t*)mmap(NULL, read_cache_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	write_cache_size = sizeof(write_handler_t) * archsim::translate::profile::RegionArch::PageCount;
	write_cache = (write_handler_t*)mmap(NULL, write_cache_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	read_user_cache_size = sizeof(read_handler_t) * archsim::translate::profile::RegionArch::PageCount;
	read_user_cache = (read_handler_t*)mmap(NULL, read_cache_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	write_user_cache_size = sizeof(write_handler_t) * archsim::translate::profile::RegionArch::PageCount;
	write_user_cache = (write_handler_t*)mmap(NULL, write_cache_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (archsim::options::LazyMemoryModelInvalidation) {
		assert(false);
		mem_model->GetCPU()->GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)read_cache, read_cache_size, this, HandleSegFaultRead);
		mem_model->GetCPU()->GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)write_cache, write_cache_size, this, HandleSegFaultWrite);
	} else {
		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++) {
			read_cache[i] = DefaultReadHandlerShunt;
			write_cache[i] = DefaultWriteHandlerSave;

			read_user_cache[i] = DefaultReadUserHandler;
			write_user_cache[i] = DefaultWriteUserHandler;
		}
	}

	auto &pubsub = mem_model->GetCPU()->GetEmulationModel().GetSystem().GetPubSub();
	subscriber = new util::PubSubscriber(&pubsub);
	subscriber->Subscribe(PubSubType::ITlbFullFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::ITlbEntryFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::DTlbFullFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::DTlbEntryFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::RegionDispatchedForTranslationVirtual, TlbCallback, this);

	subscriber->Subscribe(PubSubType::UserEvent1, TimerCallback, this);
	subscriber->Subscribe(PubSubType::UserEvent2, TimerCallback, this);

	Flush();

	mem_model->GetCPU()->state.smm_read_cache = (void*)read_cache;
	mem_model->GetCPU()->state.smm_write_cache = (void*)write_cache;
	mem_model->GetCPU()->state.smm_read_user_cache = (void*)read_user_cache;
	mem_model->GetCPU()->state.smm_write_user_cache = (void*)write_user_cache;
	mem_model->GetCPU()->state.mem_generation = 0;

	return true;
}


bool FunctionGenSystemMemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventRead, address, (uint8_t)width);
	}

	auto &types = insn_ctx.block.region.txln.types;

	llvm::Type *load_ty = NULL;
	switch(width) {
		case 1:
			load_ty = types.pi8;
			break;
		case 2:
			load_ty = types.pi16;
			break;
		case 4:
			load_ty = types.pi32;
			break;
		default:
			assert(false);
	}

	llvm::Value *fn_table = GetReadFunctionTable(insn_ctx.block.region);
	llvm::Value *this_fun = GetFunFor(insn_ctx.block.region, fn_table, address);
	llvm::Value *rval = CallFn(insn_ctx.block.region, this_fun, address);

	fault = GetFault(insn_ctx.block.region, rval);
	llvm::Value *host_ptr = GetHostPtr(insn_ctx.block.region, rval);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	llvm::Value *faulty = builder.CreateICmpNE(fault, llvm::ConstantInt::get(types.i32, 0, false));

	llvm::BasicBlock *success_block = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

	insn_ctx.block.region.EmitLeaveInstruction(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, insn_ctx.tiu.GetOffset(), faulty, success_block);

	builder.SetInsertPoint(success_block);

	host_ptr = builder.CreatePointerCast(host_ptr, load_ty);
	llvm::Value *data = builder.CreateLoad(host_ptr);
	insn_ctx.AddAliasAnalysisNode((llvm::Instruction*)host_ptr, archsim::translate::llvm::TAG_MEM_ACCESS);
	data = builder.CreateZExt(data, destinationType);
	builder.CreateStore(data, destination);

	return true;
}

bool FunctionGenSystemMemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventWrite, address, (uint8_t)width);
	}

	auto &types = insn_ctx.block.region.txln.types;

	llvm::Type *load_ty = NULL;
	switch(width) {
		case 1:
			load_ty = types.pi8;
			break;
		case 2:
			load_ty = types.pi16;
			break;
		case 4:
			load_ty = types.pi32;
			break;
		default:
			assert(false);
	}

	llvm::Value *fn_table = GetWriteFunctionTable(insn_ctx.block.region);
	llvm::Value *this_fun = GetFunFor(insn_ctx.block.region, fn_table, address);
	llvm::Value *rval = CallFn(insn_ctx.block.region, this_fun, address);

	fault = GetFault(insn_ctx.block.region, rval);
	llvm::Value *host_ptr = GetHostPtr(insn_ctx.block.region, rval);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	llvm::Value *faulty = builder.CreateICmpNE(fault, llvm::ConstantInt::get(types.i32, 0, false));

	llvm::BasicBlock *success_block = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

	insn_ctx.block.region.EmitLeaveInstruction(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, insn_ctx.tiu.GetOffset(), faulty, success_block);

	builder.SetInsertPoint(success_block);

	host_ptr = builder.CreatePointerCast(host_ptr, load_ty);
	insn_ctx.AddAliasAnalysisNode((llvm::Instruction*)host_ptr, archsim::translate::llvm::TAG_MEM_ACCESS);
	builder.CreateStore(value, host_ptr);

	return true;
}

bool FunctionGenSystemMemoryTranslationModel::EmitNonPrivilegedRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventRead, address, (uint8_t)width);
	}

	auto &types = insn_ctx.block.region.txln.types;

	llvm::Type *load_ty = NULL;
	switch(width) {
		case 1:
			load_ty = types.pi8;
			break;
		case 2:
			load_ty = types.pi16;
			break;
		case 4:
			load_ty = types.pi32;
			break;
		default:
			assert(false);
	}

	llvm::Value *fn_table = GetReadUserFunctionTable(insn_ctx.block.region);
	llvm::Value *this_fun = GetFunFor(insn_ctx.block.region, fn_table, address);
	llvm::Value *rval = CallFn(insn_ctx.block.region, this_fun, address);

	fault = GetFault(insn_ctx.block.region, rval);
	llvm::Value *host_ptr = GetHostPtr(insn_ctx.block.region, rval);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	llvm::Value *faulty = builder.CreateICmpNE(fault, llvm::ConstantInt::get(types.i32, 0, false));

	llvm::BasicBlock *success_block = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

	insn_ctx.block.region.EmitLeaveInstruction(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, insn_ctx.tiu.GetOffset(), faulty, success_block);

	builder.SetInsertPoint(success_block);

	host_ptr = builder.CreatePointerCast(host_ptr, load_ty);
	llvm::Value *data = builder.CreateLoad(host_ptr);
	insn_ctx.AddAliasAnalysisNode((llvm::Instruction*)host_ptr, archsim::translate::llvm::TAG_MEM_ACCESS);
	data = builder.CreateZExt(data, destinationType);
	builder.CreateStore(data, destination);

	return true;
}

bool FunctionGenSystemMemoryTranslationModel::EmitNonPrivilegedWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventWrite, address, (uint8_t)width);
	}

	auto &types = insn_ctx.block.region.txln.types;

	llvm::Type *load_ty = NULL;
	switch(width) {
		case 1:
			load_ty = types.pi8;
			break;
		case 2:
			load_ty = types.pi16;
			break;
		case 4:
			load_ty = types.pi32;
			break;
		default:
			assert(false);
	}

	llvm::Value *fn_table = GetWriteUserFunctionTable(insn_ctx.block.region);
	llvm::Value *this_fun = GetFunFor(insn_ctx.block.region, fn_table, address);
	llvm::Value *rval = CallFn(insn_ctx.block.region, this_fun, address);

	fault = GetFault(insn_ctx.block.region, rval);
	llvm::Value *host_ptr = GetHostPtr(insn_ctx.block.region, rval);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	llvm::Value *faulty = builder.CreateICmpNE(fault, llvm::ConstantInt::get(types.i32, 0, false));

	llvm::BasicBlock *success_block = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

	insn_ctx.block.region.EmitLeaveInstruction(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, insn_ctx.tiu.GetOffset(), faulty, success_block);

	builder.SetInsertPoint(success_block);

	host_ptr = builder.CreatePointerCast(host_ptr, load_ty);
	insn_ctx.AddAliasAnalysisNode((llvm::Instruction*)host_ptr, archsim::translate::llvm::TAG_MEM_ACCESS);
	builder.CreateStore(value, host_ptr);

	return true;
}

bool FunctionGenSystemMemoryTranslationModel::EmitPerformTranslation(archsim::translate::llvm::LLVMRegionTranslationContext &region, llvm::Value *virt_addr, llvm::Value *&phys_addr, llvm::Value *&fault)
{
	llvm::Value *fn_table = GetReadFunctionTable(region);
	llvm::Value *this_fun = GetFunFor(region, fn_table, virt_addr);

	llvm::StructType *ret_type = llvm::StructType::get(region.txln.types.i32, region.txln.types.pi8, NULL);
	llvm::Type *paramtypes[] { region.txln.types.state_ptr, region.txln.types.i32, region.txln.types.i64 };
	llvm::FunctionType *fun_type = llvm::FunctionType::get(ret_type, paramtypes, false);

	this_fun = region.builder.CreatePtrToInt(this_fun, region.txln.types.i64);
	this_fun = region.builder.CreateAdd(this_fun, llvm::ConstantInt::get(region.txln.types.i64, 10));

	this_fun = region.builder.CreateIntToPtr(this_fun, fun_type->getPointerTo(0));

	llvm::Value *rval = region.builder.CreateCall3(this_fun, region.values.state_val, virt_addr, llvm::ConstantInt::get(region.txln.types.i64, 0));
	fault = GetFault(region, rval);
	phys_addr = GetHostPtr(region, rval);
	phys_addr = region.builder.CreatePtrToInt(phys_addr, region.txln.types.i64);
	phys_addr = region.builder.CreateTrunc(phys_addr, region.txln.types.i32);

	return true;
}

archsim::util::Counter64 fn_flush;
void FunctionGenSystemMemoryTranslationModel::Flush()
{
	mem_model->FlushCaches();
}

void FunctionGenSystemMemoryTranslationModel::Evict(virt_addr_t virt_addr)
{
	if(!dirty) return;
	uint32_t i = archsim::translate::profile::RegionArch::PageIndexOf(virt_addr);

	read_cache[i] = DefaultReadHandlerShunt;
	write_cache[i] = DefaultWriteHandlerSave;
	read_user_cache[i] = DefaultReadUserHandler;
	write_user_cache[i] = DefaultWriteUserHandler;
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::GetReadFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& region)
{
	llvm::IRBuilder<> &builder = region.builder;


	auto &types = region.txln.types;

	llvm::StructType *ret_type = llvm::StructType::get(types.i32, types.pi8, NULL);
	llvm::Type *paramtypes[] { types.state_ptr, types.i32 };
	llvm::FunctionType *fun_type = llvm::FunctionType::get(ret_type, paramtypes, false);

	llvm::PointerType *fn_ptr = fun_type->getPointerTo(0)->getPointerTo(0);

	llvm::Value *table = builder.CreateStructGEP(region.values.state_val, gensim::CpuStateEntries::CpuState_smm_read_cache);
	table = builder.CreateLoad(table);
	table = builder.CreatePointerCast(table, fn_ptr);

	return table;
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::GetReadUserFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& region)
{
	llvm::IRBuilder<> &builder = region.builder;


	auto &types = region.txln.types;

	llvm::StructType *ret_type = llvm::StructType::get(types.i32, types.pi8, NULL);
	llvm::Type *paramtypes[] { types.state_ptr, types.i32 };
	llvm::FunctionType *fun_type = llvm::FunctionType::get(ret_type, paramtypes, false);

	llvm::PointerType *fn_ptr = fun_type->getPointerTo(0)->getPointerTo(0);

	llvm::Value *table = builder.CreateStructGEP(region.values.state_val, gensim::CpuStateEntries::CpuState_smm_read_user_cache);
	table = builder.CreateLoad(table);
	table = builder.CreatePointerCast(table, fn_ptr);

	return table;
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::GetWriteFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& region)
{
	llvm::IRBuilder<> &builder = region.builder;

	auto &types = region.txln.types;

	llvm::StructType *ret_type = llvm::StructType::get(types.i32, types.pi8, NULL);
	llvm::Type *paramtypes[] { types.state_ptr, types.i32 };
	llvm::FunctionType *fun_type = llvm::FunctionType::get(ret_type, paramtypes, false);

	llvm::PointerType *fn_ptr = fun_type->getPointerTo(0)->getPointerTo(0);

	llvm::Value *table = builder.CreateStructGEP(region.values.state_val, gensim::CpuStateEntries::CpuState_smm_write_cache);
	table = builder.CreateLoad(table);
	table = builder.CreatePointerCast(table, fn_ptr);

	return table;
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::GetWriteUserFunctionTable(archsim::translate::llvm::LLVMRegionTranslationContext& region)
{
	llvm::IRBuilder<> &builder = region.builder;

	auto &types = region.txln.types;

	llvm::StructType *ret_type = llvm::StructType::get(types.i32, types.pi8, NULL);
	llvm::Type *paramtypes[] { types.state_ptr, types.i32 };
	llvm::FunctionType *fun_type = llvm::FunctionType::get(ret_type, paramtypes, false);

	llvm::PointerType *fn_ptr = fun_type->getPointerTo(0)->getPointerTo(0);

	llvm::Value *table = builder.CreateStructGEP(region.values.state_val, gensim::CpuStateEntries::CpuState_smm_write_user_cache);
	table = builder.CreateLoad(table);
	table = builder.CreatePointerCast(table, fn_ptr);

	return table;
}
llvm::Value *FunctionGenSystemMemoryTranslationModel::GetFunFor(archsim::translate::llvm::LLVMRegionTranslationContext& region, llvm::Value *fn_table, llvm::Value *address)
{
	assert(Complete);

	llvm::IRBuilder<> &builder = region.builder;

	address = builder.CreateLShr(address, archsim::translate::profile::RegionArch::PageBits);
	llvm::Value *fn = builder.CreateGEP(fn_table, address);
	fn = builder.CreateLoad(fn);

	return fn;
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::CallFn(archsim::translate::llvm::LLVMRegionTranslationContext& region, llvm::Value *fn, llvm::Value *address)
{
	llvm::IRBuilder<> &builder = region.builder;

	return builder.CreateCall2(fn, region.values.state_val, address);
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::GetFault(archsim::translate::llvm::LLVMRegionTranslationContext& region, llvm::Value *rval)
{
	llvm::IRBuilder<> &builder = region.builder;
	unsigned int indices[] = {0};
	return builder.CreateExtractValue(rval, indices);
}

llvm::Value *FunctionGenSystemMemoryTranslationModel::GetHostPtr(archsim::translate::llvm::LLVMRegionTranslationContext& region, llvm::Value *rval)
{
	llvm::IRBuilder<> &builder = region.builder;
	unsigned int indices[] = {1};
	return builder.CreateExtractValue(rval, indices);
}
