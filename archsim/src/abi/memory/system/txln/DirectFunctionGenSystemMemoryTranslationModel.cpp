/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * FunctionGenSystemMemoryTranslationModel.cpp
 *
 *  Created on: 9 Sep 2014
 *      Author: harry
 */

#include "abi/memory/MemoryEventHandlerTranslator.h"
#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/MMU.h"
#include "gensim/gensim_processor.h"
#include "translate/TranslationWorkUnit.h"
#include "ij/arch/x86/macros.h"
#include "util/PubSubSync.h"

using namespace archsim::abi::memory;

class DirectFunctionGenSystemMemoryTranslationModel : public SystemMemoryTranslationModel
{
public:
	struct memfunc_rval {
		uint32_t fault;
		uint8_t *ptr;
	};

	typedef memfunc_rval (*read_handler_t)(cpuState *, uint32_t);
	typedef memfunc_rval (*write_handler_t)(cpuState *, uint32_t);

	DirectFunctionGenSystemMemoryTranslationModel();

	virtual bool Initialise(SystemMemoryModel &mem_model) override;

	bool EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination) override;
	bool EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value) override;

	void Flush() override;
	void Evict(virt_addr_t virt_addr) override;

	read_handler_t GenerateAndInstallReadFunction(gensim::Processor *cpu, uint32_t address);
	write_handler_t GenerateAndInstallWriteFunction(gensim::Processor *cpu, uint32_t address);

	// This indicates whether the translation model should create one function for every page in memory, or act as a cache
	// Right now only Complete (one function per page) mode is supported
	bool Complete;

private:
	bool dirty;

	llvm::Value *GetReadFunctionTable(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx);
	llvm::Value *GetWriteFunctionTable(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx);

	llvm::Value *GetFunFor(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *fn_table, llvm::Value *address);
	llvm::Value *CallFn(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *fn, llvm::Value *address);
	llvm::Value *GetFault(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *rval);
	llvm::Value *GetHostPtr(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *rval);

	// Zone for storing generated functions
	typedef uint8_t fn_storage_t[64];
	std::bitset<1024*1024> dirty_map;

	fn_storage_t *r_fn_storage;
	fn_storage_t *w_fn_storage;

	size_t r_fn_storage_size, w_fn_storage_size;

	SystemMemoryModel *mem_model;
	archsim::util::PubSubscriber *subscriber;

};

UseLogContext(LogSystemMemoryModel);
DeclareChildLogContext(LogDirectFunctionSystemMemoryModel, LogSystemMemoryModel, "DirectFunction");

static DirectFunctionGenSystemMemoryTranslationModel *singleton;

static DirectFunctionGenSystemMemoryTranslationModel::memfunc_rval DefaultReadHandler(cpuState *cpu, uint32_t address)
{
	assert(singleton->Complete);

	gensim::Processor *proc = (gensim::Processor*)cpu->cpu_ctx;
	DirectFunctionGenSystemMemoryTranslationModel::read_handler_t fn = singleton->GenerateAndInstallReadFunction(proc, address);
	DirectFunctionGenSystemMemoryTranslationModel::memfunc_rval rval = fn(cpu, address);
#ifdef CHECK_FUNCTIONS
	auto smm = (SystemMemoryModel *)&proc->GetMemoryModel();
	uint32_t phys_addr;
	auto tx_result = smm->GetMMU().Translate(proc, address, phys_addr, cpu->in_kern_mode, false);
	host_addr_t host_addr;
	auto ptr = proc->GetEmulationModel().GetMemoryModel().LockRegion(phys_addr, 4, host_addr);

	uint8_t data;
	auto read_rval = smm->Read8(address, data);

	if(rval.fault) {
		if(!smm->GetDeviceManager().HasDevice(phys_addr))
			//either both are below 1024, or both are >= 1024
			assert((read_rval < 1024 && rval.fault < 1024 && tx_result < 1024) || (read_rval >= 1024 && rval.fault >= 1024 && tx_result >= 1024));
	} else
		assert(rval.fault == read_rval && rval.fault == tx_result && rval.ptr == (uint8_t*)host_addr);
#endif
	return rval;
}

static DirectFunctionGenSystemMemoryTranslationModel::memfunc_rval DefaultWriteHandler(cpuState *cpu, uint32_t address)
{
	assert(singleton->Complete);

	gensim::Processor *proc = (gensim::Processor*)cpu->cpu_ctx;
	DirectFunctionGenSystemMemoryTranslationModel::write_handler_t fn = singleton->GenerateAndInstallWriteFunction(proc, address);
	DirectFunctionGenSystemMemoryTranslationModel::memfunc_rval rval = fn(cpu, address);

#ifdef CHECK_FUNCTIONS
	auto smm = (SystemMemoryModel *)&proc->GetMemoryModel();
	uint32_t phys_addr;
	auto tx_result = smm->GetMMU().Translate(proc, address, phys_addr, cpu->in_kern_mode, false);
	host_addr_t host_addr;
	auto ptr = proc->GetEmulationModel().GetMemoryModel().LockRegion(phys_addr, 4, host_addr);

	auto write_rval = smm->Write8(address, 0);

	if(write_rval) {
		archsim::translate::profile::Region* rgn;
		if(!smm->GetDeviceManager().HasDevice(phys_addr)) {
			if(!smm->GetCpu().GetEmulationModel().GetSystem().GetTranslationManager().TryGetRegion(address, rgn) && rgn->HasTranslations())
				//either both are below 1024, or both are >= 1024
				assert((rval.fault < 1024 && tx_result < 1024 && write_rval < 1024) || (rval.fault >= 1024 && tx_result >= 1024 && write_rval >= 1024));
		}
	} else
		assert(rval.fault == write_rval && rval.fault == tx_result && rval.ptr == (uint8_t*)host_addr);
#endif
	return rval;
}

static void EmitDefaultRead(uint8_t *mem_buffer)
{
	auto rip = x86::EMIT_RIP_MOV_OP(mem_buffer, x86::RegRAX);
	x86::EMIT_BYTE(mem_buffer, 0xff);
	x86::EMIT_BYTE(mem_buffer, 0xe0);
	x86::EMIT_RIP_CONSTANT(mem_buffer, rip, (uint64_t)&DefaultReadHandler);
}

static void EmitDefaultWrite(uint8_t *mem_buffer)
{
	auto rip = x86::EMIT_RIP_MOV_OP(mem_buffer, x86::RegRAX);
	x86::EMIT_BYTE(mem_buffer, 0xff);
	x86::EMIT_BYTE(mem_buffer, 0xe0);
	x86::EMIT_RIP_CONSTANT(mem_buffer, rip, (uint64_t)&DefaultWriteHandler);
}


DirectFunctionGenSystemMemoryTranslationModel::DirectFunctionGenSystemMemoryTranslationModel() : dirty(true)
{

}

static bool HandleSegFaultRead(void *ctx, const System::segfault_data& data)
{
	uint8_t *page_base = (uint8_t *)(data.addr & ~4095ULL);
	uint8_t *page_end = page_base + 4096;
	mprotect(page_base, 4096, PROT_READ | PROT_WRITE);

	for(uint8_t *ptr = (uint8_t*)page_base; ptr < page_end; ptr += 64) EmitDefaultRead(ptr);

	return true;
}

static bool HandleSegFaultWrite(void *ctx, const System::segfault_data& data)
{
	uint8_t *page_base = (uint8_t *)(data.addr & ~4095ULL);
	uint8_t *page_end = page_base + 4096;
	mprotect(page_base, 4096, PROT_READ | PROT_WRITE);

	for(uint8_t *ptr = (uint8_t*)page_base; ptr < page_end; ptr += 64) EmitDefaultWrite(ptr);

	return true;
}

static void TlbCallback(PubSubType::PubSubType type, void *ctx, void *data)
{
	auto fgen = (DirectFunctionGenSystemMemoryTranslationModel*)ctx;
	if(type == PubSubType::ITlbFullFlush || type == PubSubType::DTlbFullFlush) fgen->Flush();
	else {
		virt_addr_t addr = (virt_addr_t) (uint64_t)data;
		fgen->Evict(addr);
	}
}

bool DirectFunctionGenSystemMemoryTranslationModel::Initialise(SystemMemoryModel &mem_model)
{
	Complete = true;
	singleton = this;
	this->mem_model = &mem_model;

	int mmap_prot = PROT_READ | PROT_WRITE;

	if (archsim::options::LazyMemoryModelInvalidation) {
		mmap_prot = PROT_NONE;
	}

	r_fn_storage_size = sizeof(fn_storage_t) * archsim::translate::profile::RegionArch::PageCount;
	r_fn_storage = (fn_storage_t*)mmap(NULL, r_fn_storage_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	w_fn_storage_size = sizeof(fn_storage_t) * archsim::translate::profile::RegionArch::PageCount;
	w_fn_storage = (fn_storage_t*)mmap(NULL, w_fn_storage_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (archsim::options::LazyMemoryModelInvalidation) {
		mem_model.GetCpu().GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)r_fn_storage, r_fn_storage_size, this, HandleSegFaultRead);
		mem_model.GetCpu().GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)w_fn_storage, w_fn_storage_size, this, HandleSegFaultWrite);
	}

	auto &pubsub = mem_model.GetCpu().GetEmulationModel().GetSystem().GetPubSub();
	subscriber = new archsim::util::PubSubscriber(&pubsub);
	subscriber->Subscribe(PubSubType::ITlbFullFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::ITlbEntryFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::DTlbFullFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::DTlbEntryFlush, TlbCallback, this);
	subscriber->Subscribe(PubSubType::RegionDispatchedForTranslationPhysical, TlbCallback, this);

	Flush();

	mem_model.GetCpu().state.smm_read_cache = (void*)r_fn_storage;
	mem_model.GetCpu().state.smm_write_cache = (void*)w_fn_storage;

	return true;
}

bool DirectFunctionGenSystemMemoryTranslationModel::EmitMemoryRead(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination)
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

	llvm::Value *fn_table = GetReadFunctionTable(insn_ctx);
	llvm::Value *this_fun = GetFunFor(insn_ctx, fn_table, address);
	llvm::Value *rval = CallFn(insn_ctx, this_fun, address);

	fault = GetFault(insn_ctx, rval);
	llvm::Value *host_ptr = GetHostPtr(insn_ctx, rval);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	llvm::Value *faulty = builder.CreateICmpNE(fault, llvm::ConstantInt::get(types.i32, 0, false));

	llvm::BasicBlock *success_block = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

	insn_ctx.block.region.EmitCondLeave(false, faulty, success_block, archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, true);

	builder.SetInsertPoint(success_block);

	host_ptr = builder.CreatePointerCast(host_ptr, load_ty);
	llvm::Value *data = builder.CreateLoad(host_ptr);
	data = builder.CreateZExt(data, destinationType);
	builder.CreateStore(data, destination);

	return true;
}

bool DirectFunctionGenSystemMemoryTranslationModel::EmitMemoryWrite(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value)
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

	llvm::Value *fn_table = GetWriteFunctionTable(insn_ctx);
	llvm::Value *this_fun = GetFunFor(insn_ctx, fn_table, address);
	llvm::Value *rval = CallFn(insn_ctx, this_fun, address);

	fault = GetFault(insn_ctx, rval);
	llvm::Value *host_ptr = GetHostPtr(insn_ctx, rval);

	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	llvm::Value *faulty = builder.CreateICmpNE(fault, llvm::ConstantInt::get(types.i32, 0, false));

	llvm::BasicBlock *success_block = llvm::BasicBlock::Create(builder.getContext(), "", builder.GetInsertBlock()->getParent());

	insn_ctx.block.region.EmitCondLeave(false, faulty, success_block, archsim::translate::llvm::LLVMRegionTranslationContext::RequireInterpret, true);

	builder.SetInsertPoint(success_block);

	host_ptr = builder.CreatePointerCast(host_ptr, load_ty);
	builder.CreateStore(value, host_ptr);

	return true;
}

void DirectFunctionGenSystemMemoryTranslationModel::Flush()
{

	if (!dirty)
		return;
	dirty = false;

	if (archsim::options::LazyMemoryModelInvalidation) {
		mprotect(r_fn_storage, r_fn_storage_size, PROT_NONE);
		mprotect(w_fn_storage, w_fn_storage_size, PROT_NONE);
	} else {
		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++)
			EmitDefaultRead(r_fn_storage[i]);

		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++)
			EmitDefaultWrite(w_fn_storage[i]);
	}
}

void DirectFunctionGenSystemMemoryTranslationModel::Evict(virt_addr_t virt_addr)
{
	if(!dirty) return;
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(virt_addr);
	EmitDefaultRead(r_fn_storage[page_index]);
	EmitDefaultWrite(w_fn_storage[page_index]);
}

DirectFunctionGenSystemMemoryTranslationModel::read_handler_t DirectFunctionGenSystemMemoryTranslationModel::GenerateAndInstallReadFunction(gensim::Processor *cpu, uint32_t address)
{
	uint32_t page_base_address = archsim::translate::profile::RegionArch::PageBaseOf(address);
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(address);

	uint8_t *orig_mem_buffer = (uint8_t*)r_fn_storage[page_index];
	uint8_t *mem_buffer = orig_mem_buffer;

	LC_DEBUG1(LogDirectFunctionSystemMemoryModel) << "Generating read function for VA " << std::hex << page_base_address;

	auto pi = mem_model->GetMMU().GetInfo(address);
	uint32_t phys_address = (pi.phys_addr & ~pi.mask) | (address & pi.mask);
	uint32_t phys_page_base = archsim::translate::profile::RegionArch::PageBaseOf(phys_address);
	if(!pi.Present) {
		LC_DEBUG1(LogDirectFunctionSystemMemoryModel) << " - Page not present";
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	}
	if(mem_model->GetDeviceManager().HasDevice(phys_address)) {
		LC_DEBUG1(LogDirectFunctionSystemMemoryModel) << " - Page is device";
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1024, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else {
		LC_DEBUG1(LogDirectFunctionSystemMemoryModel) << " - Page present, PA " << std::hex << phys_page_base;
		// Incoming register values:
		//   RDI = CPU State Ptr
		//   RSI = Virtual Address
		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bxor, x86::reg::RegEAX,x86::reg::RegEAX);
		if(!pi.UserCanRead) {
			LC_DEBUG1(LogDirectFunctionSystemMemoryModel) << " - Page requires kernel priv to read";
			x86::EMIT_MOV_OP(mem_buffer, x86::MemAccessInfo::get(x86::reg::RegRDI, offsetof(cpuState, in_kern_mode)), x86::reg::RegAL);
			x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bxor, (uint8_t)1, x86::reg::RegEAX);
		}


		host_addr_t out;
		if(!cpu->GetEmulationModel().GetMemoryModel().LockRegion(phys_page_base, archsim::translate::profile::RegionArch::PageSize, out)) assert(false);

		uint8_t *rip = x86::EMIT_RIP_MOV_OP(mem_buffer, x86::reg::RegRDX);

		// mask the incoming address to get the page offset in ESI
		// andl PAGEMASK, %esi
		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_band, (uint32_t)archsim::translate::profile::RegionArch::PageMask, x86::reg::RegESI);

		// leaq (%rdx, %rsi, 1), %rdx
//		x86::EMIT_LEA_OP(mem_buffer, x86::MemAccessInfo::get(x86::reg::RegRDX, x86::reg::RegRSI, 1, 0), x86::reg::RegRDX);
		// or %rsi, %rdx
		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bor, x86::reg::RegRSI, x86::reg::RegRDX);

		// retq
		x86::EMIT_BYTE(mem_buffer, 0xc3);
		x86::EMIT_RIP_CONSTANT(mem_buffer, rip, (uint64_t)out);
	}

	assert((mem_buffer - orig_mem_buffer) <= 64);

	if(archsim::options::Debug) {
		std::stringstream str;
		str << "mmap-fun-r-" << std::hex << page_base_address << ".bin";
		FILE *f = fopen(str.str().c_str(), "wt");
		fwrite(orig_mem_buffer, 1, mem_buffer-orig_mem_buffer, f);
		fclose(f);
	}

	LC_DEBUG1(LogDirectFunctionSystemMemoryModel) << " - " << (mem_buffer - orig_mem_buffer) << " bytes emitted";

	dirty = true;

	return (read_handler_t)orig_mem_buffer;
}

DirectFunctionGenSystemMemoryTranslationModel::read_handler_t DirectFunctionGenSystemMemoryTranslationModel::GenerateAndInstallWriteFunction(gensim::Processor *cpu, uint32_t address)
{
	uint32_t page_base_address = archsim::translate::profile::RegionArch::PageBaseOf(address);
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(address);

	uint8_t *orig_mem_buffer = (uint8_t*)w_fn_storage[page_index];
	uint8_t *mem_buffer = orig_mem_buffer;

	auto pi = mem_model->GetMMU().GetInfo(page_base_address);
	uint32_t phys_address = (pi.phys_addr & ~pi.mask) | (address & pi.mask);
	uint32_t phys_page_base = archsim::translate::profile::RegionArch::PageBaseOf(phys_address);

	if(!pi.Present) {
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else if(mem_model->GetDeviceManager().HasDevice(phys_page_base) || mem_model->GetCpu().GetEmulationModel().GetSystem().GetTranslationManager().GetRegion(phys_page_base).HasTranslations()) {
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1024, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else {
		// Incoming register values:
		//   RDI = CPU State Ptr
		//   RSI = Virtual Address

		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bxor, x86::reg::RegEAX,x86::reg::RegEAX);

		if(!pi.KernelCanWrite && !pi.UserCanWrite) {
			x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
			x86::EMIT_BYTE(mem_buffer, 0xc3);
		} else {
			if(!pi.UserCanWrite) {
				x86::EMIT_MOV_OP(mem_buffer, x86::MemAccessInfo::get(x86::reg::RegRDI, offsetof(cpuState, in_kern_mode)), x86::reg::RegAL);
				x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bxor, (uint8_t)1, x86::reg::RegEAX);
			}
			host_addr_t out;
			if(!cpu->GetEmulationModel().GetMemoryModel().LockRegion(phys_page_base, archsim::translate::profile::RegionArch::PageSize, out)) assert(false);

			uint8_t *rip = x86::EMIT_RIP_MOV_OP(mem_buffer, x86::reg::RegRDX);

			// mask the incoming address to get the page offset in ESI
			// andl PAGEMASK, %esi
			x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_band, (uint32_t)archsim::translate::profile::RegionArch::PageMask, x86::reg::RegESI);

			// leaq (%rdx, %rsi, 1), %rdx
			// x86::EMIT_LEA_OP(mem_buffer, x86::MemAccessInfo::get(x86::reg::RegRDX, x86::reg::RegRSI, 1, 0), x86::reg::RegRDX);

			// or %rsi, %rdx
			x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bor, x86::reg::RegRSI, x86::reg::RegRDX);

			// retq
			x86::EMIT_BYTE(mem_buffer, 0xc3);
			x86::EMIT_RIP_CONSTANT(mem_buffer, rip, (uint64_t)out);
		}
	}

	assert((mem_buffer - orig_mem_buffer) <= 64);

	if(archsim::options::Debug) {
		std::stringstream str;
		str << "mmap-fun-w-" << std::hex << page_base_address << ".bin";
		FILE *f = fopen(str.str().c_str(), "wt");
		fwrite(orig_mem_buffer, 1, mem_buffer-orig_mem_buffer, f);
		fclose(f);
	}

	dirty = true;

	return (read_handler_t)orig_mem_buffer;
}


llvm::Value *DirectFunctionGenSystemMemoryTranslationModel::GetReadFunctionTable(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx)
{
	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	auto &types = insn_ctx.block.region.txln.types;

	llvm::Type *buf_type = llvm::ArrayType::get(types.i8, 64);
	buf_type = buf_type->getPointerTo(0);

	llvm::Value *table = builder.CreateStructGEP(insn_ctx.block.region.values.state_val, gensim::CpuStateEntries::CpuState_smm_read_cache);
	table = builder.CreateLoad(table);
	table = builder.CreatePointerCast(table, buf_type);

	return table;
}

llvm::Value *DirectFunctionGenSystemMemoryTranslationModel::GetWriteFunctionTable(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx)
{
	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	auto &types = insn_ctx.block.region.txln.types;

	llvm::Type *buf_type = llvm::ArrayType::get(types.i8, 64);
	buf_type = buf_type->getPointerTo(0);

	llvm::Value *table = builder.CreateStructGEP(insn_ctx.block.region.values.state_val, gensim::CpuStateEntries::CpuState_smm_write_cache);
	table = builder.CreateLoad(table);
	table = builder.CreatePointerCast(table, buf_type);

	return table;
}

llvm::Value *DirectFunctionGenSystemMemoryTranslationModel::GetFunFor(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *fn_table, llvm::Value *address)
{
	assert(Complete);

	auto &types = insn_ctx.block.region.txln.types;
	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;

	address = builder.CreateLShr(address, archsim::translate::profile::RegionArch::PageBits);
	llvm::Value *fn = builder.CreateGEP(fn_table, address);

	llvm::StructType *ret_type = llvm::StructType::get(types.i32, types.pi8, NULL);
	llvm::Type *paramtypes[] { types.state_ptr, types.i32 };
	llvm::Type *fun_type = llvm::FunctionType::get(ret_type, paramtypes, false)->getPointerTo(0);

	return builder.CreateBitCast(fn, fun_type);
}

llvm::Value *DirectFunctionGenSystemMemoryTranslationModel::CallFn(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *fn, llvm::Value *address)
{
	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;
	llvm::CallInst *call = builder.CreateCall2(fn, insn_ctx.block.region.values.state_val, address);
	call->setOnlyReadsMemory();
	return call;
}

llvm::Value *DirectFunctionGenSystemMemoryTranslationModel::GetFault(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *rval)
{
	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;
	unsigned int indices[] = {0};
	return builder.CreateExtractValue(rval, indices);
}

llvm::Value *DirectFunctionGenSystemMemoryTranslationModel::GetHostPtr(archsim::translate::llvm::LLVMInstructionTranslationContext& insn_ctx, llvm::Value *rval)
{
	llvm::IRBuilder<> &builder = insn_ctx.block.region.builder;
	unsigned int indices[] = {1};
	return builder.CreateExtractValue(rval, indices);
}
