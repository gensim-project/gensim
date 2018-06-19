/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "abi/memory/MemoryEventHandler.h"
#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/MMU.h"
#include "gensim/gensim_processor.h"
#include "translate/Translation.h"
#include "translate/TranslationWorkUnit.h"
#include "util/ComponentManager.h"

#include <sys/mman.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IRBuilder.h>

using namespace archsim::abi::memory;
using namespace archsim::translate::profile;

static bool HandleCacheSegfault(void *ctx, const System::segfault_data& data)
{
	void *host_virt_page = (void *)(data.addr & ~4095);

	mprotect(host_virt_page, 4096, PROT_READ | PROT_WRITE);
	bzero(host_virt_page, 4096);

	return true;
}

static void TlbCallback(PubSubType::PubSubType type, void *ctx, void *data)
{
	auto fgen = (DirectMappedSystemMemoryTranslationModel*)ctx;
	switch(type) {
		case PubSubType::ITlbFullFlush:
		case PubSubType::PrivilegeLevelChange:
			fgen->Flush();
			break;
		case PubSubType::ITlbEntryFlush:
		case PubSubType::RegionDispatchedForTranslationPhysical:
			fgen->Evict((virt_addr_t)(uint64_t)data);
			break;
		default:
			assert(false);

	}
}

bool DirectMappedSystemMemoryTranslationModel::Initialise(SystemMemoryModel& mem_model)
{
	uint32_t entries = archsim::translate::profile::RegionArch::PageCount;

	int mmap_prot = PROT_READ | PROT_WRITE;;

	if (archsim::options::LazyMemoryModelInvalidation) {
		mmap_prot = PROT_NONE;
	}

	mem_model.GetCpu().state.smm_read_cache = mmap(NULL, entries * sizeof(void *), mmap_prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (mem_model.GetCpu().state.smm_read_cache == MAP_FAILED) {
		return false;
	}

	mem_model.GetCpu().state.smm_write_cache = mmap(NULL, entries * sizeof(void *), mmap_prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (mem_model.GetCpu().state.smm_write_cache == MAP_FAILED) {
		return false;
	}

	if (archsim::options::LazyMemoryModelInvalidation) {
		mem_model.GetCpu().GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)mem_model.GetCpu().state.smm_read_cache, entries * sizeof(void *), this, HandleCacheSegfault);
		mem_model.GetCpu().GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)mem_model.GetCpu().state.smm_write_cache, entries * sizeof(void *), this, HandleCacheSegfault);
	}

	auto &pubsub = mem_model.GetCpu().GetEmulationModel().GetSystem().GetPubSub();
	subscriber = new util::PubSubscriber(&pubsub);
	subscriber->Subscribe(PubSubType::ITlbFullFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::ITlbEntryFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::RegionDispatchedForTranslationPhysical, TlbCallback, this);
	subscriber->Subscribe(PubSubType::PrivilegeLevelChange, TlbCallback, this);

	this->mem_model = &mem_model;
	return true;
}

struct CacheMissResult {
	uint32_t rc;
	host_addr_t page_base;
};

extern "C"
CacheMissResult DirectCacheMissRead(gensim::Processor& cpu, uint32_t address)
{
	CacheMissResult result;
	uint32_t phys_addr;

	result.rc = ((SystemMemoryModel &)cpu.GetMemoryModel()).GetMMU().Translate(&cpu, address, phys_addr, MMUACCESSINFO(cpu.in_kernel_mode(), 0), false);

	if (result.rc == 0 && !((SystemMemoryModel &)cpu.GetMemoryModel()).GetDeviceManager().HasDevice(phys_addr)) {
		host_addr_t host_addr;
		cpu.GetEmulationModel().GetMemoryModel().LockRegion(RegionArch::PageBaseOf(phys_addr), RegionArch::PageSize, host_addr);

		uint32_t pg_idx = RegionArch::PageIndexOf(address);
		((host_addr_t *)cpu.state.smm_read_cache)[pg_idx] = host_addr;
		result.page_base = host_addr;
	} else if (result.rc == 0) {
		result.rc = TXLN_RESULT_MUST_INTERPRET;
	}

	return result;
}

extern "C"
CacheMissResult DirectCacheMissWrite(gensim::Processor& cpu, uint32_t address)
{
	CacheMissResult result;
	uint32_t phys_addr;

	result.rc = ((SystemMemoryModel &)cpu.GetMemoryModel()).GetMMU().Translate(&cpu, address, phys_addr, MMUACCESSINFO(cpu.in_kernel_mode(), 1), false);

	if (result.rc == 0 && !((SystemMemoryModel &)cpu.GetMemoryModel()).GetDeviceManager().HasDevice(phys_addr)) {
		host_addr_t host_addr;
		cpu.GetEmulationModel().GetMemoryModel().LockRegion(RegionArch::PageBaseOf(phys_addr), RegionArch::PageSize, host_addr);

		uint32_t pg_idx = RegionArch::PageIndexOf(address);
		((host_addr_t *)cpu.state.smm_write_cache)[pg_idx] = host_addr;
		result.page_base = host_addr;
	} else if (result.rc == 0) {
		result.rc = TXLN_RESULT_MUST_INTERPRET;
	}

	return result;
}

bool DirectMappedSystemMemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
{
	// Emit memory read event handlers
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventRead, address, (uint8_t)width);
	}

	auto& builder = insn_ctx.block.region.builder;
	auto& txln = insn_ctx.block.region.txln;

	auto pg_table_ptr = builder.CreatePointerCast(builder.CreateStructGEP(insn_ctx.block.region.values.state_val, gensim::CpuStateEntries::CpuState_smm_read_cache), txln.types.pi8->getPointerTo(0)->getPointerTo(0));
	pg_table_ptr = builder.CreateLoad(pg_table_ptr);

	auto pg_index = builder.CreateLShr(address, txln.GetConstantInt32(RegionArch::PageBits));

	auto pg_ptr = builder.CreateLoad(builder.CreateGEP(pg_table_ptr, pg_index));
	auto pg_ptr_null = builder.CreateICmpEQ(builder.CreatePtrToInt(pg_ptr, txln.types.i64), txln.GetConstantInt64(0));

	::llvm::BasicBlock *miss_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "miss", insn_ctx.block.region.region_fn);
	::llvm::BasicBlock *cont_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "cont", insn_ctx.block.region.region_fn);

	auto entry_block = builder.GetInsertBlock();
	builder.CreateCondBr(pg_ptr_null, miss_block, cont_block);

	builder.SetInsertPoint(miss_block);

	auto miss_fn_rettype = ::llvm::StructType::get(txln.types.i32, txln.types.pi8, NULL);
	auto miss_fn = txln.llvm_module->getOrInsertFunction("DirectCacheMissRead", miss_fn_rettype, txln.types.cpu_ctx, txln.types.i32, NULL);
	auto miss_result = builder.CreateCall2(miss_fn, insn_ctx.block.region.values.cpu_ctx_val, address);

	unsigned int indices0[] = {0};
	auto miss_rc = builder.CreateExtractValue(miss_result, indices0);

	unsigned int indices1[] = {1};
	auto miss_pg_ptr = builder.CreateExtractValue(miss_result, indices1);

	builder.CreateCondBr(builder.CreateICmpEQ(miss_rc, txln.GetConstantInt32(0)), cont_block, insn_ctx.block.region.GetExitBlock(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret));

	builder.SetInsertPoint(cont_block);

	::llvm::Value *incoming_pg_ptr = builder.CreatePHI(txln.types.pi8, 2);
	((::llvm::PHINode *)incoming_pg_ptr)->addIncoming(pg_ptr, entry_block);
	((::llvm::PHINode *)incoming_pg_ptr)->addIncoming(miss_pg_ptr, miss_block);

	auto pg_off = builder.CreateAnd(address, txln.GetConstantInt32(RegionArch::PageMask));

	::llvm::Value *pg_idx;
	switch(width) {
		case 4:
			pg_idx = builder.CreateLShr(pg_off, 2);
			incoming_pg_ptr = builder.CreatePointerCast(incoming_pg_ptr, txln.types.pi32);
			break;
		case 2:
			pg_idx = builder.CreateLShr(pg_off, 1);
			incoming_pg_ptr = builder.CreatePointerCast(incoming_pg_ptr, txln.types.pi16);
			break;
		case 1:
			pg_idx = pg_off;
			incoming_pg_ptr = builder.CreatePointerCast(incoming_pg_ptr, txln.types.pi8);
			break;
		default:
			return false;
	}

	auto gep = builder.CreateGEP(incoming_pg_ptr, pg_idx);
	auto val = builder.CreateLoad(gep);
	auto casted_val = builder.CreateCast(sx ? ::llvm::Instruction::SExt : ::llvm::Instruction::ZExt, val, destinationType);

	builder.CreateStore(casted_val, destination);
	fault = txln.GetConstantInt32(0);

	return true;
}

bool DirectMappedSystemMemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
{
	// Emit memory write event handlers
	for (auto handler : insn_ctx.block.region.txln.twu.GetProcessor().GetMemoryModel().GetEventHandlers()) {
		handler->GetTranslator().EmitEventHandler(insn_ctx, *handler, archsim::abi::memory::MemoryModel::MemEventWrite, address, (uint8_t)width);
	}

	auto& builder = insn_ctx.block.region.builder;
	auto& txln = insn_ctx.block.region.txln;

	auto pg_table_ptr = builder.CreatePointerCast(builder.CreateStructGEP(insn_ctx.block.region.values.state_val, gensim::CpuStateEntries::CpuState_smm_write_cache), txln.types.pi8->getPointerTo(0)->getPointerTo(0));
	pg_table_ptr = builder.CreateLoad(pg_table_ptr);

	auto pg_index = builder.CreateLShr(address, txln.GetConstantInt32(RegionArch::PageBits));

	::llvm::Value *pg_ptr = builder.CreateLoad(builder.CreateGEP(pg_table_ptr, pg_index));

	auto pg_ptr_null = builder.CreateICmpEQ(builder.CreatePtrToInt(pg_ptr, txln.types.i64), txln.GetConstantInt64(0));

	::llvm::BasicBlock *cont_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "cont", insn_ctx.block.region.region_fn);
	::llvm::BasicBlock *miss_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "miss", insn_ctx.block.region.region_fn);

	auto entry_block = builder.GetInsertBlock();
	builder.CreateCondBr(pg_ptr_null, miss_block, cont_block);

	fault = txln.GetConstantInt32(0);

	/// MISS
	builder.SetInsertPoint(miss_block);

	auto miss_fn_rettype = ::llvm::StructType::get(txln.types.i32, txln.types.pi8, NULL);
	auto miss_fn = txln.llvm_module->getOrInsertFunction("DirectCacheMissWrite", miss_fn_rettype, txln.types.cpu_ctx, txln.types.i32, NULL);
	auto miss_result = builder.CreateCall2(miss_fn, insn_ctx.block.region.values.cpu_ctx_val, address);

	unsigned int indices0[] = {0};
	auto miss_rc = builder.CreateExtractValue(miss_result, indices0);

	unsigned int indices1[] = {1};
	auto miss_pg_ptr = builder.CreateExtractValue(miss_result, indices1);

	builder.CreateCondBr(builder.CreateICmpEQ(miss_rc, txln.GetConstantInt32(0)), cont_block, insn_ctx.block.region.GetExitBlock(archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret));

	/// CONT
	builder.SetInsertPoint(cont_block);

	::llvm::Value *incoming_pg_ptr = builder.CreatePHI(txln.types.pi8, 2);
	((::llvm::PHINode *)incoming_pg_ptr)->addIncoming(pg_ptr, entry_block);
	((::llvm::PHINode *)incoming_pg_ptr)->addIncoming(miss_pg_ptr, miss_block);

	auto pg_off = builder.CreateAnd(address, txln.GetConstantInt32(RegionArch::PageMask));

	::llvm::Value *pg_idx;
	switch(width) {
		case 4:
			pg_idx = builder.CreateLShr(pg_off, 2);
			incoming_pg_ptr = builder.CreatePointerCast(incoming_pg_ptr, txln.types.pi32);
			break;
		case 2:
			pg_idx = builder.CreateLShr(pg_off, 1);
			incoming_pg_ptr = builder.CreatePointerCast(incoming_pg_ptr, txln.types.pi16);
			break;
		case 1:
			pg_idx = pg_off;
			incoming_pg_ptr = builder.CreatePointerCast(incoming_pg_ptr, txln.types.pi8);
			break;
		default:
			return false;
	}

	auto gep = builder.CreateGEP(incoming_pg_ptr, pg_idx);
	builder.CreateStore(value, gep);

	return true;
}

void DirectMappedSystemMemoryTranslationModel::Flush()
{
	if (archsim::options::LazyMemoryModelInvalidation) {
		mprotect(mem_model->GetCpu().state.smm_read_cache, sizeof(void *) * archsim::translate::profile::RegionArch::PageCount, PROT_NONE);
		mprotect(mem_model->GetCpu().state.smm_write_cache, sizeof(void *) * archsim::translate::profile::RegionArch::PageCount, PROT_NONE);
	} else {
		bzero(mem_model->GetCpu().state.smm_read_cache, sizeof(void *) * archsim::translate::profile::RegionArch::PageCount);
		bzero(mem_model->GetCpu().state.smm_write_cache, sizeof(void *) * archsim::translate::profile::RegionArch::PageCount);
	}
}

void DirectMappedSystemMemoryTranslationModel::Evict(virt_addr_t virt_addr)
{
	Flush();
}
