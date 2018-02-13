/*
 * FunctionBasedSystemMemoryModel.cpp
 *
 *  Created on: 22 Apr 2015
 *      Author: harry
 */

#include "abi/Address.h"
#include "abi/memory/system/FunctionBasedSystemMemoryModel.h"
#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "gensim/gensim_processor.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/MMU.h"
#include "util/LogContext.h"
#include "util/ComponentManager.h"
#include "ij/arch/x86/macros.h"
//#include "blockjit/PerfMap.h"

UseLogContext(LogMemoryModel);
UseLogContext(LogSystemMemoryModel);
DeclareChildLogContext(LogFunctionSystemMemoryModel, LogSystemMemoryModel, "Function");

using namespace archsim::abi::memory;

RegisterComponent(SystemMemoryModel, FunctionBasedSystemMemoryModel, "function", "Function generation based memory model", MemoryModel*, archsim::util::PubSubContext *);

extern "C" FunctionBasedSystemMemoryModel::memfunc_rval DefaultReadHandlerShunt(cpuState *cpustate, uint32_t address);
extern "C" FunctionBasedSystemMemoryModel::memfunc_rval DefaultWriteHandlerSave(cpuState *cpustate, uint32_t address);

extern "C" FunctionBasedSystemMemoryModel::memfunc_rval DefaultReadHandler(cpuState *cpustate, uint32_t address)
{
	gensim::Processor *proc = (gensim::Processor*)cpustate->cpu_ctx;
	FunctionBasedSystemMemoryModel *fn_model = (FunctionBasedSystemMemoryModel *)&proc->GetMemoryModel();
	assert(fn_model->Complete);
	FunctionBasedSystemMemoryModel::read_handler_t fn = fn_model->GenerateAndInstallReadFunction(proc, address, 0);
	FunctionBasedSystemMemoryModel::memfunc_rval rval = fn(cpustate, address);

	return rval;
}

extern "C" FunctionBasedSystemMemoryModel::memfunc_rval DefaultReadUserHandler(cpuState *cpustate, uint32_t address)
{
	gensim::Processor *proc = (gensim::Processor*)cpustate->cpu_ctx;
	FunctionBasedSystemMemoryModel *fn_model = (FunctionBasedSystemMemoryModel *)&proc->GetMemoryModel();
	assert(fn_model->Complete);
	FunctionBasedSystemMemoryModel::read_handler_t fn = fn_model->GenerateAndInstallReadFunction(proc, address, 1);
	FunctionBasedSystemMemoryModel::memfunc_rval rval = fn(cpustate, address);

	return rval;
}

extern "C" FunctionBasedSystemMemoryModel::memfunc_rval DefaultTxlnHandler(cpuState *cpustate, uint32_t address, uint64_t zero)
{
	gensim::Processor *proc = (gensim::Processor*)cpustate->cpu_ctx;
	FunctionBasedSystemMemoryModel *fn_model = (FunctionBasedSystemMemoryModel *)&proc->GetMemoryModel();
	assert(fn_model->Complete);
	FunctionBasedSystemMemoryModel::read_handler_t fn = fn_model->GenerateAndInstallReadFunction(proc, address, 0);

	FunctionBasedSystemMemoryModel::translate_handler_t txln = (FunctionBasedSystemMemoryModel::translate_handler_t)(((char*)fn)+10);

	FunctionBasedSystemMemoryModel::memfunc_rval rval = txln(cpustate, address, 0);

	return rval;
}

extern "C" FunctionBasedSystemMemoryModel::memfunc_rval DefaultWriteHandler(cpuState *cpustate, uint32_t address)
{
	gensim::Processor *proc = (gensim::Processor*)cpustate->cpu_ctx;
	FunctionBasedSystemMemoryModel *fn_model = (FunctionBasedSystemMemoryModel *)&proc->GetMemoryModel();
	assert(fn_model->Complete);
	FunctionBasedSystemMemoryModel::write_handler_t fn = fn_model->GenerateAndInstallWriteFunction(proc, address, 0);
	FunctionBasedSystemMemoryModel::memfunc_rval rval = fn(cpustate, address);

	return rval;
}

FunctionBasedSystemMemoryModel::memfunc_rval DefaultWriteUserHandler(cpuState *cpustate, uint32_t address)
{
	gensim::Processor *proc = (gensim::Processor*)cpustate->cpu_ctx;
	FunctionBasedSystemMemoryModel *fn_model = (FunctionBasedSystemMemoryModel *)&proc->GetMemoryModel();
	assert(fn_model->Complete);
	FunctionBasedSystemMemoryModel::write_handler_t fn = fn_model->GenerateAndInstallWriteFunction(proc, address, 1);
	FunctionBasedSystemMemoryModel::memfunc_rval rval = fn(cpustate, address);

	return rval;
}

static void FlushCallback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	SystemMemoryModel *smm = (SystemMemoryModel*)ctx;
	FunctionBasedSystemMemoryModel *cmm = (FunctionBasedSystemMemoryModel*)smm;
	switch(type) {
		case PubSubType::ITlbEntryFlush:
		case PubSubType::DTlbEntryFlush:
			cmm->EvictCacheEntry((uint32_t)(uint64_t)data);
			break;
		case PubSubType::RegionDispatchedForTranslationPhysical:
		case PubSubType::ITlbFullFlush:
		case PubSubType::DTlbFullFlush:
			cmm->FlushCaches();
			break;
		case PubSubType::PrivilegeLevelChange:
			cmm->InstallCaches();
			break;
		default:
			assert(false);
	}
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

FunctionBasedSystemMemoryModel::FunctionBasedSystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub) : SystemMemoryModel(phys_mem, pubsub), _subscriber(pubsub), base_mem(phys_mem, pubsub), Complete(false)
{
	base_mem.SetUnalignedBehaviour(BaseSystemMemoryModel::Unaligned_EMULATE);
#if CONFIG_LLVM
	translation_model = new FunctionGenSystemMemoryTranslationModel();
#endif
}

FunctionBasedSystemMemoryModel::~FunctionBasedSystemMemoryModel() {}

bool FunctionBasedSystemMemoryModel::Initialise()
{
	base_mem.SetCPU(GetCPU());
	base_mem.SetDeviceManager(GetDeviceManager());
	base_mem.SetMMU(GetMMU());


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
		GetCPU()->GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)read_cache, read_cache_size, this, HandleSegFaultRead);
		GetCPU()->GetEmulationModel().GetSystem().RegisterSegFaultHandler((uint64_t)write_cache, write_cache_size, this, HandleSegFaultWrite);
	} else {
		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++) {
			read_cache[i] = DefaultReadHandlerShunt;
			write_cache[i] = DefaultWriteHandlerSave;

			read_user_cache[i] = DefaultReadUserHandler;
			write_user_cache[i] = DefaultWriteUserHandler;
		}
	}

	_subscriber.Subscribe(PubSubType::RegionDispatchedForTranslationPhysical, FlushCallback, this);
	_subscriber.Subscribe(PubSubType::DTlbEntryFlush, FlushCallback, this);
	_subscriber.Subscribe(PubSubType::DTlbFullFlush, FlushCallback, this);
	_subscriber.Subscribe(PubSubType::PrivilegeLevelChange, FlushCallback, this);


	InstallCaches();
	GetCPU()->state.mem_generation = 0;

	Complete = true;

#if CONFIG_LLVM
	translation_model->Initialise(*this);
#endif

	return true;
}

void FunctionBasedSystemMemoryModel::InstallCaches()
{
	if(GetCPU()->in_kernel_mode()) {
		GetCPU()->state.smm_read_cache = (void*)read_cache;
		GetCPU()->state.smm_write_cache = (void*)write_cache;
	} else {
		GetCPU()->state.smm_read_cache = (void*)read_user_cache;
		GetCPU()->state.smm_write_cache = (void*)write_user_cache;
	}

	GetCPU()->state.smm_read_user_cache = (void*)read_user_cache;
	GetCPU()->state.smm_write_user_cache = (void*)write_user_cache;
}


void FunctionBasedSystemMemoryModel::Destroy()
{

}

void FunctionBasedSystemMemoryModel::FlushCaches()
{
	if (!dirty)
		return;
	dirty = false;

	LC_DEBUG1(LogFunctionSystemMemoryModel) << "Flushed";

	if (archsim::options::LazyMemoryModelInvalidation) {
		mprotect(read_cache, read_cache_size, PROT_NONE);
		mprotect(write_cache, write_cache_size, PROT_NONE);

		r_fn_zone.Clear();
		w_fn_zone.Clear();
	} else {
		assert(GetCPU());
		GetCPU()->state.mem_generation++;
		check_overflow();
	}
}

void FunctionBasedSystemMemoryModel::EvictCacheEntry(virt_addr_t virt_addr)
{
#if CONFIG_LLVM
	translation_model->Evict(virt_addr);
#endif

	uint32_t page_idx = archsim::Address(virt_addr).GetPageIndex();

	read_cache[page_idx] = DefaultReadHandlerShunt;
	write_cache[page_idx] = DefaultWriteHandlerSave;

	read_user_cache[page_idx] = DefaultReadUserHandler;
	write_user_cache[page_idx] = DefaultWriteUserHandler;
}

MemoryTranslationModel &FunctionBasedSystemMemoryModel::GetTranslationModel()
{
#if CONFIG_LLVM
	return *translation_model;
#else
	abort();
#endif
}

uint32_t FunctionBasedSystemMemoryModel::Read(guest_addr_t guest_addr, uint8_t *data, int size)
{
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((read_handler_t*)(GetCPU()->state.smm_read_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);

	if(rval.fault) {
		return base_mem.Read(guest_addr, data, size);
	} else {
		switch(size) {
			case 1:
				*(uint8_t*)(data) = *rval.ptr;
				break;
			case 2:
				*(uint16_t*)(data) = *(uint16_t*)rval.ptr;
				break;
			case 4:
				*(uint32_t*)(data) = *(uint32_t*)rval.ptr;
				break;
			default:
				assert(false);
				break;
		}
		return 0;
	}
}

uint32_t FunctionBasedSystemMemoryModel::Read8_zx(guest_addr_t guest_addr, uint32_t &data)
{
	LC_DEBUG3(LogSystemMemoryModel) << "Read8_zx";
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((read_handler_t*)(GetCPU()->state.smm_read_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);
	if(rval.fault) {
		return base_mem.Read8_zx(guest_addr, data);
	} else {
		data = *rval.ptr;
		return 0;
	}

}

uint32_t FunctionBasedSystemMemoryModel::Read32(guest_addr_t guest_addr, uint32_t &data)
{
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((read_handler_t*)(GetCPU()->state.smm_read_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);

	if(UNLIKELY(rval.fault)) {
		return base_mem.Read32(guest_addr, data);
	} else {
		data = *(uint32_t*)rval.ptr;
		return 0;
	}
}


uint32_t FunctionBasedSystemMemoryModel::Write32(guest_addr_t guest_addr, uint32_t data)
{
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	write_handler_t handler = ((read_handler_t*)(GetCPU()->state.smm_write_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);

	if(UNLIKELY(rval.fault)) {
		return base_mem.Write32(guest_addr, data);
	} else {
		*(uint32_t*)rval.ptr = data;
		return 0;
	}
}

uint32_t FunctionBasedSystemMemoryModel::Write(guest_addr_t guest_addr, uint8_t *data, int size)
{
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	write_handler_t handler = ((write_handler_t*)(GetCPU()->state.smm_write_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);

	if(rval.fault) {
		uint32_t temp = base_mem.Write(guest_addr, data, size);
		return temp;
	} else {
		switch(size) {
			case 1:
				*(uint8_t*)(rval.ptr) = *data;
				break;
			case 2:
				*(uint16_t*)(rval.ptr) = *(uint16_t*)data;
				break;
			case 4:
				*(uint32_t*)(rval.ptr) = *(uint32_t*)data;
				break;
			default:
				assert(false);
				break;
		}
		return 0;
	}
}
uint32_t FunctionBasedSystemMemoryModel::Fetch(guest_addr_t guest_addr, uint8_t *data, int size)
{
	return base_mem.Fetch(guest_addr, data, size);
}
uint32_t FunctionBasedSystemMemoryModel::Peek(guest_addr_t guest_addr, uint8_t *data, int size)
{
	return base_mem.Peek(guest_addr, data, size);
}
uint32_t FunctionBasedSystemMemoryModel::Poke(guest_addr_t guest_addr, uint8_t *data, int size)
{
	return base_mem.Poke(guest_addr, data, size);
}

uint32_t FunctionBasedSystemMemoryModel::Fetch32(guest_addr_t guest_addr, uint32_t &data)
{
	return base_mem.Fetch32(guest_addr, data);
}

uint32_t FunctionBasedSystemMemoryModel::Read8User(guest_addr_t guest_addr, uint32_t&data)
{
	LC_DEBUG3(LogSystemMemoryModel) << "Read8User";
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((read_handler_t*)(GetCPU()->state.smm_read_user_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);
	if(rval.fault) {
		return base_mem.Read8User(guest_addr, data);
	} else {
		data = *(uint8_t*)rval.ptr;
		return 0;
	}
}

uint32_t FunctionBasedSystemMemoryModel::Read32User(guest_addr_t guest_addr, uint32_t&data)
{
	LC_DEBUG3(LogSystemMemoryModel) << "Read32User";
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((read_handler_t*)(GetCPU()->state.smm_read_user_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);
	if(rval.fault) {
		return base_mem.Read32User(guest_addr, data);
	} else {
		data = *(uint32_t*)rval.ptr;
		return 0;
	}
}

uint32_t FunctionBasedSystemMemoryModel::Write8User(guest_addr_t guest_addr, uint8_t data)
{
	LC_DEBUG3(LogSystemMemoryModel) << "Write8User";
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((write_handler_t*)(GetCPU()->state.smm_write_user_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);
	if(rval.fault) {
		return base_mem.Write8User(guest_addr, data);
	} else {
		*(uint8_t*)rval.ptr = data;
		return 0;
	}
}

uint32_t FunctionBasedSystemMemoryModel::Write32User(guest_addr_t guest_addr, uint32_t data)
{
	LC_DEBUG3(LogSystemMemoryModel) << "Write32User";
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(guest_addr);
	read_handler_t handler = ((write_handler_t*)(GetCPU()->state.smm_write_user_cache))[page_index];
	memfunc_rval rval = handler(&GetCPU()->state, guest_addr);
	if(rval.fault) {
		return base_mem.Write32User(guest_addr, data);
	} else {
		*(uint32_t*)rval.ptr = data;
		return 0;
	}
}

bool FunctionBasedSystemMemoryModel::ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr)
{
	return false;
}

uint32_t FunctionBasedSystemMemoryModel::PerformTranslation(virt_addr_t virt_addr, phys_addr_t &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info)
{
	return base_mem.PerformTranslation(virt_addr, out_phys_addr, info);

	/*
	LC_DEBUG3(LogSystemMemoryModel) << "PerformTranslation " << std::hex << this->GetCPU()->read_pc();
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(virt_addr);
	read_handler_t read_handler = ((read_handler_t*)(GetCPU()->state.smm_read_cache))[page_index];
	char *fn_ptr = ((char*)read_handler) + 10;
	translate_handler_t handler = (translate_handler_t)fn_ptr;
	memfunc_rval rval = handler(&GetCPU()->state, virt_addr, 0);

	if(rval.fault) {
		return base_mem.PerformTranslation(virt_addr, out_phys_addr, info);
	}

	out_phys_addr = (uint32_t)(uint64_t)rval.ptr;
	return 0;
	 */
}

void FunctionBasedSystemMemoryModel::check_overflow()
{
	if(GetCPU()->state.mem_generation == 0xffffffff) {
		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++)
			read_cache[i] = DefaultReadHandlerShunt;

		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++)
			read_user_cache[i] = DefaultReadUserHandler;

		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++)
			write_cache[i] = DefaultWriteHandlerSave;

		for (uint32_t i = 0; i < archsim::translate::profile::RegionArch::PageCount; i++)
			write_user_cache[i] = DefaultWriteUserHandler;

		r_fn_zone.Clear();
		w_fn_zone.Clear();
		r_user_fn_zone.Clear();
		w_user_fn_zone.Clear();

		GetCPU()->state.mem_generation = 0;
	}
}


/**
 * Calling convention for memory access functions
 * Incoming arguments:
 * CPU State Pointer		%RDI
 * Guest virtual address	%ESI
 *
 * Results:
 * Fault status				%EAX
 * Host virtual address		%RDX
 *
 * Clobbered regs:
 * %RAX, %RDX, %RSI
 */

FunctionBasedSystemMemoryModel::read_handler_t FunctionBasedSystemMemoryModel::GenerateAndInstallReadFunction(gensim::Processor *cpu, uint32_t address, bool is_user_only)
{

	uint32_t page_base_address = archsim::translate::profile::RegionArch::PageBaseOf(address);
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(address);

	read_handler_t *target_ptr;
	if(is_user_only) target_ptr = &read_user_cache[page_index];
	else target_ptr = &read_cache[page_index];
	if(*target_ptr == DefaultReadUserHandler || *target_ptr == DefaultReadHandlerShunt) {
		*target_ptr = (read_handler_t)r_fn_zone.Get(page_base_address);
	}
	uint8_t *orig_mem_buffer = (uint8_t*)*target_ptr;
	uint8_t *mem_buffer = orig_mem_buffer;

	LC_DEBUG1(LogFunctionSystemMemoryModel) << "Generating read function for VA " << std::hex << page_base_address << " (cpu pc: " << cpu->read_pc() << ", approx insn " << std::dec << cpu->instructions() << ")";

	const auto pi = GetMMU()->GetInfo(address);
	uint32_t phys_address = (pi.phys_addr & ~pi.mask) | (address & pi.mask);
	uint32_t phys_page_base = archsim::translate::profile::RegionArch::PageBaseOf(phys_address);

	uint8_t *generational_j_offset = NULL;
	uint8_t *j_base = NULL;

	bool present = pi.Present;
	bool device = GetDeviceManager()->HasDevice(phys_address);
	bool perm_check = !pi.UserCanRead || !pi.KernelCanRead;

	// This function has multiple entry points so some operations must be done right at the top
	host_addr_t out;
	//Only attempt to get a host pointer if the guest page is actually present
	if(present) {
		if(!cpu->GetEmulationModel().GetMemoryModel().LockRegion(phys_page_base, archsim::translate::profile::RegionArch::PageSize, out)) assert(false);
	} else {
		out = 0;
	}

	uint8_t *pre_mem_buffer = mem_buffer;
	if((uint64_t)out < UINT32_MAX) {
		x86::EMIT_MOV_OP(mem_buffer, ((uint32_t)(uint64_t)out), x86::reg::RegEDX);
	} else {
		x86::EMIT_MOV_OP(mem_buffer, ((uint64_t)out), x86::reg::RegRDX);
	}

	//Load the memory function generation from the CPU state and compare against the generation in which this function was made.
	//If they differ, then this function is invalid and should be replaced (branch to trail	er).

	LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Emitting generational invalidation code";
//	x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_cmp, (uint32_t)cpu->state.mem_generation, x86::MemAccessInfo::get(x86::reg::RegRDI, offsetof(cpuState, mem_generation)));
//	generational_j_offset = x86::EMIT_WEE_J(mem_buffer, x86::X86OpType::o_jne, 0);
	j_base = mem_buffer;

	if(!pi.Present) {
		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page not present";
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else if(device) {
		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page is device";
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1024, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else {
		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page present, PA " << std::hex << phys_page_base;
		// Incoming register values:
		//   RDI = CPU State Ptr
		//   RSI = Virtual Address

		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bxor, x86::reg::RegEAX,x86::reg::RegEAX);
		if(!pi.UserCanRead) {
			if(is_user_only) {
				x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::reg::RegEAX);
			} else {
				LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page requires kernel priv to read";

				// we don't need to do this, as we only access kernel pages from the kernel cache (which is only in kernel mode)
//				x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_cmp, (uint8_t)1, x86::MemAccessInfo::get(x86::reg::RegRDI, offsetof(cpuState, in_kern_mode)));
//				x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_adc, x86::reg::RegEAX, x86::reg::RegEAX);
			}
		}

		// mask the incoming address to get the page offset in ESI
		// andl PAGEMASK, %esi
		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_band, (uint32_t)archsim::translate::profile::RegionArch::PageMask, x86::reg::RegESI);

		// leaq (%rdx, %rsi, 1), %rdx
//		x86::EMIT_LEA_OP(mem_buffer, x86::MemAccessInfo::get(x86::reg::RegRDX, x86::reg::RegRSI, 1, 0), x86::reg::RegRDX);
		// or %rsi, %rdx
		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bor, x86::reg::RegRSI, x86::reg::RegRDX);

		// retq
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	}

#ifndef MEM_FUNC_LAZY_INVAL
	//If this function has been invalidated, tail-call the default handler to create a new function

	LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Emitting generational code trailer";
	if(generational_j_offset && j_base) {
		*generational_j_offset = (uint32_t)(mem_buffer-j_base);
	}

	if(is_user_only) {
		x86::EMIT_MOV_OP(mem_buffer, (uint64_t)DefaultReadUserHandler, x86::RegRAX);
	} else {
		x86::EMIT_MOV_OP(mem_buffer, (uint64_t)DefaultReadHandlerShunt, x86::RegRAX);
	}
	x86::EMIT_IND_JMP(mem_buffer, x86::RegRAX);
#endif

	assert((mem_buffer - orig_mem_buffer) <= r_fn_zone.BlockSize);

#define DEBUG_FNS
#ifdef DEBUG_FNS
#ifndef NDEBUG
	if(archsim::options::Debug) {
		std::stringstream str;
		str << "mmap-fun-r-" << std::hex << page_base_address << ".bin";
		FILE *f = fopen(str.str().c_str(), "wt");
		fwrite(orig_mem_buffer, 1, mem_buffer-orig_mem_buffer, f);
		fclose(f);
	}
#endif
#endif

	LC_DEBUG1(LogFunctionSystemMemoryModel) << " - " << (mem_buffer - orig_mem_buffer) << " bytes emitted";
	/*
		PerfMap &pmap = PerfMap::Singleton;
		if(pmap.Enabled()) {
			pmap.Acquire();
			pmap.Stream() << std::hex << (size_t)orig_mem_buffer << " " << (mem_buffer - orig_mem_buffer) << " MemReadFunction" << std::endl;
			pmap.Release();
		}
		*/
	dirty = true;
	return (read_handler_t)orig_mem_buffer;
}

FunctionBasedSystemMemoryModel::read_handler_t FunctionBasedSystemMemoryModel::GenerateAndInstallWriteFunction(gensim::Processor *cpu, uint32_t address, bool is_user_only)
{
	uint32_t page_base_address = archsim::translate::profile::RegionArch::PageBaseOf(address);
	uint32_t page_index = archsim::translate::profile::RegionArch::PageIndexOf(address);

	write_handler_t *buffer_location;
	if(is_user_only) buffer_location = &write_user_cache[page_index];
	else buffer_location = &write_cache[page_index];

	if(*buffer_location == DefaultWriteHandlerSave || *buffer_location == DefaultWriteUserHandler) {
		*buffer_location = (read_handler_t)w_fn_zone.Get(page_base_address);
	}
	uint8_t *orig_mem_buffer = (uint8_t*)*buffer_location;
	uint8_t *mem_buffer = orig_mem_buffer;

	auto pi = GetMMU()->GetInfo(page_base_address);
	uint32_t phys_address = (pi.phys_addr & ~pi.mask) | (address & pi.mask);
	uint32_t phys_page_base = archsim::translate::profile::RegionArch::PageBaseOf(phys_address);

	archsim::translate::profile::Region *rgn;

	uint8_t *generational_j_offset = NULL;
	uint8_t *j_base = NULL;

	LC_DEBUG1(LogFunctionSystemMemoryModel) << "Generating write function for VA " << std::hex << page_base_address << " (cpu pc: " << cpu->read_pc() << ", approx insn " << std::dec << cpu->instructions() << ")";

	if(!archsim::options::LazyMemoryModelInvalidation) {
		//Load the memory function generation from the CPU state and compare against the generation in which this function was made.
		//If they differ, then this function is invalid and should be replaced (branch to trailer).

		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Emitting generational invalidation code";

		x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_cmp, (uint32_t)cpu->state.mem_generation, x86::MemAccessInfo::get(x86::reg::RegRDI, offsetof(cpuState, mem_generation)));
		generational_j_offset = x86::EMIT_WEE_J(mem_buffer, x86::X86OpType::o_jne, 0);
		j_base = mem_buffer;
	}

	// XXX HAX
	bool page_is_code_or_device = false;
	if(pi.Present) {
		if(GetDeviceManager()->HasDevice(phys_page_base)) {
			page_is_code_or_device = true;
		}

		if(GetCPU()->GetEmulationModel().GetSystem().HaveTranslationManager() && GetCPU()->GetEmulationModel().GetSystem().GetTranslationManager().TryGetRegion(phys_page_base, rgn)) {
			page_is_code_or_device = true;
		}

		if(GetProfile().IsRegionCode(archsim::PhysicalAddress(phys_page_base))) {
			page_is_code_or_device = true;
		}
	}

	if(!pi.Present) {
		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page not present. Signalling memory error.";
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else if(page_is_code_or_device) {
		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page is device or code. Signalling TXLN_MUST_INTERPRET.";
		x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1024, x86::RegEAX);
		x86::EMIT_BYTE(mem_buffer, 0xc3);
	} else {
		// Incoming register values:
		//   RDI = CPU State Ptr
		//   RSI = Virtual Address

		if(!pi.KernelCanWrite && !pi.UserCanWrite) {
			LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page is read only. Signalling memory error.";
			x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
			x86::EMIT_BYTE(mem_buffer, 0xc3);
		} else {
			x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bxor, x86::reg::RegEAX,x86::reg::RegEAX);
			if(!pi.UserCanWrite) {
				if(is_user_only) {
					x86::EMIT_MOV_OP(mem_buffer, (uint32_t)1, x86::RegEAX);
				} else {
					LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Page is user read only.";
					x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_cmp, (uint8_t)1, x86::MemAccessInfo::get(x86::reg::RegRDI, offsetof(cpuState, in_kern_mode)));
					x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_adc, x86::reg::RegEAX, x86::reg::RegEAX);
				}
			}

			host_addr_t out;
			if(!cpu->GetEmulationModel().GetMemoryModel().LockRegion(phys_page_base, archsim::translate::profile::RegionArch::PageSize, out)) assert(false);

			LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Emitting translation code.";

			x86::EMIT_MOV_OP(mem_buffer, (uint64_t)out, x86::reg::RegRDX);

			// mask the incoming address to get the page offset in ESI
			// andl PAGEMASK, %esi
			x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_band, (uint32_t)archsim::translate::profile::RegionArch::PageMask, x86::reg::RegESI);

			// leaq (%rdx, %rsi, 1), %rdx
			// x86::EMIT_LEA_OP(mem_buffer, x86::MemAccessInfo::get(x86::reg::RegRDX, x86::reg::RegRSI, 1, 0), x86::reg::RegRDX);

			// or %rsi, %rdx
			x86::EMIT_ALU_OP(mem_buffer, x86::X86OpType::o_bor, x86::reg::RegRSI, x86::reg::RegRDX);

			// retq
			x86::EMIT_BYTE(mem_buffer, 0xc3);
		}
	}

	if(!archsim::options::LazyMemoryModelInvalidation) {
		//If this function has been invalidated, tail-call the default handler to create a new function

		LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Emitting generational tail";

		*generational_j_offset = (uint32_t)(mem_buffer-j_base);
		if(is_user_only) {
			x86::EMIT_MOV_OP(mem_buffer, (uint64_t)DefaultWriteUserHandler, x86::RegRAX);
		} else {
			x86::EMIT_MOV_OP(mem_buffer, (uint64_t)DefaultWriteHandlerSave, x86::RegRAX);
		}
		x86::EMIT_IND_JMP(mem_buffer, x86::RegRAX);
	}

	assert((mem_buffer - orig_mem_buffer) <= w_fn_zone.BlockSize);

#ifdef DEBUG_FNS
#ifndef NDEBUG
	if(UNLIKELY(archsim::options::Debug)) {
		std::stringstream str;
		str << "mmap-fun-w-" << std::hex << page_base_address << ".bin";
		FILE *f = fopen(str.str().c_str(), "wt");
		fwrite(orig_mem_buffer, 1, mem_buffer-orig_mem_buffer, f);
		fclose(f);
	}
#endif
#endif

	LC_DEBUG1(LogFunctionSystemMemoryModel) << " - Installing function";
	/*
		PerfMap &pmap = PerfMap::Singleton;
		if(pmap.Enabled()) {
			pmap.Acquire();
			pmap.Stream() << std::hex << (size_t)orig_mem_buffer << " " << (mem_buffer - orig_mem_buffer) << " MemWriteFunction" << std::endl;
			pmap.Release();
		}
		*/
	dirty = true;
	return (read_handler_t)orig_mem_buffer;
}
