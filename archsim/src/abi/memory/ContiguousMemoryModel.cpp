/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "abi/memory/MemoryModel.h"
#include "abi/memory/MemoryTranslationModel.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"

#include <sys/mman.h>
#include <cstring>

#if ARCHSIM_SIMULATION_HOST_IS_x86_64
#include <asm/prctl.h>
#include <sys/prctl.h>
#endif

extern "C" int arch_prctl(int code, unsigned long addr);

#define DIRECT

UseLogContext(LogMemoryModel);
DeclareChildLogContext(LogContiguousMemory, LogMemoryModel, "ContiguousMemory");

#define CONTIGUOUS_MEMORY_SIZE 0x100000000  // 4GB

using namespace archsim::abi::memory;

RegisterComponent(MemoryModel, ContiguousMemoryModel, "contiguous", "");

inline int ProtFlags(RegionFlags prot)
{
	unsigned int flags = PROT_NONE;

	if ((prot & RegFlagRead) == RegFlagRead) flags |= PROT_READ;

	if ((prot & RegFlagWrite) == RegFlagWrite) flags |= PROT_WRITE;

	if ((prot & RegFlagExecute) == RegFlagExecute) flags |= PROT_READ;

	return flags;
}

ContiguousMemoryModel::ContiguousMemoryModel() : mem_base(NULL), translation_model(nullptr), is_initialised(false)
{
#if CONFIG_LLVM
	translation_model = new ContiguousMemoryTranslationModel();
#endif
}

ContiguousMemoryModel::~ContiguousMemoryModel()
{
#if CONFIG_LLVM
	delete translation_model;
#endif
}

bool ContiguousMemoryModel::Initialise()
{
	if (is_initialised) return true;

	// try to map the memory base pointer to somewhere sensible
	mem_base = (host_addr_t)mmap((void*)0x80000000, (size_t)CONTIGUOUS_MEMORY_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0);

#ifdef MAP_32BIT
	if(mem_base == MAP_FAILED) {
		mem_base = (host_addr_t)mmap((void*)nullptr, (size_t)CONTIGUOUS_MEMORY_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_32BIT, -1, 0);
	}
#endif

	if(mem_base == MAP_FAILED) {
		mem_base = (host_addr_t)mmap((void*)nullptr, (size_t)CONTIGUOUS_MEMORY_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	}

	if (mem_base == MAP_FAILED) {
		LC_ERROR(LogMemoryModel) << "Failed to map memory.";
		return false;
	}

	is_initialised = true;

#if ARCHSIM_SIMULATION_HOST_IS_x86_64
	if(archsim::options::SystemMemoryModel == "user") {
		if(arch_prctl(ARCH_SET_GS, (unsigned long)mem_base) == -1) {
			LC_ERROR(LogMemoryModel) << "Failed to set GS register: " << strerror(errno);
		}

		unsigned long l;
		arch_prctl(ARCH_GET_GS, (unsigned long)&l);
		assert(l == (unsigned long)mem_base);

		LC_INFO(LogMemoryModel) << "Using X86_64 specific memory model extension.";
	}
#endif

#if CONFIG_LLVM
	((ContiguousMemoryTranslationModel*)translation_model)->SetContiguousMemoryBase(mem_base);
#endif

	return true;
}

void ContiguousMemoryModel::Destroy()
{
#ifndef DIRECT
	munmap(mem_base, CONTIGUOUS_MEMORY_SIZE);
#endif
}

bool ContiguousMemoryModel::LockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t& host_addr)
{
	host_addr = GuestToHost(guest_addr);
	return true;
}

bool ContiguousMemoryModel::LockRegions(guest_addr_t guest_addr, guest_size_t guest_size, LockedMemoryRegion& regions)
{
	ASSERT(guest_addr.GetPageOffset() == 0);

	std::vector<void *> page_ptrs;
	for(Address a = guest_addr; a < guest_addr + guest_size; a += 4096) {
		page_ptrs.push_back(GuestToHost(a));
	}

	regions = LockedMemoryRegion(guest_addr, page_ptrs);
	return true;
}


bool ContiguousMemoryModel::UnlockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t host_addr)
{
	return true;
}

bool ContiguousMemoryModel::AllocateVMA(GuestVMA &vma)
{
	void *host_addr = mmap(GuestToHost(vma.base), vma.size, ProtFlags(vma.protection), MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);

	LC_DEBUG1(LogContiguousMemory) << "Allocated " << host_addr << " for vma " << vma.base;

	if (host_addr == MAP_FAILED) {
		LC_DEBUG1(LogContiguousMemory) << " - Failed! " << strerror(errno);
		return false;
	}

	vma.host_base = (host_addr_t)host_addr;

	return true;
}

bool ContiguousMemoryModel::DeallocateVMA(GuestVMA &vma)
{
	mprotect(vma.host_base, vma.size, PROT_NONE);
	return true;
}

bool ContiguousMemoryModel::ResizeVMA(GuestVMA &vma, guest_size_t new_size)
{
	// Remove all protection from the existing region, then reassert the protection.
	guest_size_t old_size = vma.size;
	mprotect(vma.host_base, vma.size, PROT_NONE);
	vma.size = new_size;
	mprotect(vma.host_base, vma.size, ProtFlags(vma.protection));

	// Zero new memory
	if (new_size > old_size) {
		bzero((void *)((unsigned long)vma.host_base + old_size), (new_size - old_size));
	}

	return true;
}

bool ContiguousMemoryModel::SynchroniseVMAProtection(GuestVMA &vma)
{
	mprotect(vma.host_base, vma.size, ProtFlags(vma.protection));
	return true;
}

MemoryTranslationModel &ContiguousMemoryModel::GetTranslationModel()
{
	return *translation_model;
}

bool ContiguousMemoryModel::ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr)
{
#ifdef DIRECT
	guest_addr = (guest_addr_t)(intptr_t)host_addr;
	return true;
#else
	if (host_addr >= (host_const_addr_t)mem_base && host_addr < (host_const_addr_t)((unsigned long)mem_base + CONTIGUOUS_MEMORY_SIZE)) {
		guest_addr = (guest_addr_t)((unsigned long)host_addr - (unsigned long)mem_base);
		return true;
	}
	return false;
#endif
}

uint32_t ContiguousMemoryModel::Read(guest_addr_t addr, uint8_t *data, int size)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventRead, addr, size);
#endif

	memcpy(data, (void *)((uintptr_t)mem_base + addr.Get()), size);
	return 0;
}

uint32_t ContiguousMemoryModel::Fetch(guest_addr_t addr, uint8_t *data, int size)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventFetch, addr, size);
#endif

	memcpy(data, (void *)((uintptr_t)mem_base + addr.Get()), size);
	return 0;
}

uint32_t ContiguousMemoryModel::Write(guest_addr_t addr, uint8_t *data, int size)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventWrite, addr, size);
#endif

	memcpy((void *)((uintptr_t)mem_base + addr.Get()), (void *)data, size);
	return 0;
}

uint32_t ContiguousMemoryModel::Peek(guest_addr_t addr, uint8_t *data, int size)
{
	RegionFlags old_flags;
	if (!GetRegionProtection(addr, old_flags)) {
		return 1;
	}

	ProtectRegion(addr, size, RegFlagRead);
	memcpy(data, (void *)((uintptr_t)mem_base + addr.Get()), size);
	ProtectRegion(addr, size, old_flags);

	return 0;
}

uint32_t ContiguousMemoryModel::Poke(guest_addr_t addr, uint8_t *data, int size)
{
	RegionFlags old_flags;
	if (!GetRegionProtection(addr, old_flags)) {
		return 1;
	}

	ProtectRegion(addr, size, RegFlagReadWrite);
	memcpy((void *)((uintptr_t)mem_base + addr.Get()), (void *)data, size);
	ProtectRegion(addr, size, old_flags);

	return 0;
}

uint32_t ContiguousMemoryModel::Read8(guest_addr_t addr, uint8_t &data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventRead, addr, 1);
#endif

	data = *(uint8_t *)((uintptr_t)mem_base + addr.Get());
	return 0;
}

uint32_t ContiguousMemoryModel::Read16(guest_addr_t addr, uint16_t &data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventRead, addr, 2);
#endif

	data = *(uint16_t *)((uintptr_t)mem_base + addr.Get());
	return 0;
}

uint32_t ContiguousMemoryModel::Read32(guest_addr_t addr, uint32_t &data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventRead, addr, 4);
#endif

	data = *(uint32_t *)((uintptr_t)mem_base + addr.Get());
	return 0;
}

uint32_t ContiguousMemoryModel::Fetch8(guest_addr_t addr, uint8_t &data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventFetch, addr, 1);
#endif

	data = *(uint8_t *)((uintptr_t)mem_base + addr.Get());
	return 0;
}

uint32_t ContiguousMemoryModel::Fetch16(guest_addr_t addr, uint16_t &data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventFetch, addr, 2);
#endif

	data = *(uint16_t *)((uintptr_t)mem_base + addr.Get());
	return 0;
}

uint32_t ContiguousMemoryModel::Fetch32(guest_addr_t addr, uint32_t &data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventFetch, addr, 4);
#endif

	data = *(uint32_t *)((uintptr_t)mem_base + addr.Get());
	return 0;
}

uint32_t ContiguousMemoryModel::Write8(guest_addr_t addr, uint8_t data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventWrite, addr, 1);
#endif

	*(uint8_t *)((uintptr_t)mem_base + addr.Get()) = data;
	return 0;
}

uint32_t ContiguousMemoryModel::Write16(guest_addr_t addr, uint16_t data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventWrite, addr, 2);
#endif

	*(uint16_t *)((uintptr_t)mem_base + addr.Get()) = data;
	return 0;
}

uint32_t ContiguousMemoryModel::Write32(guest_addr_t addr, uint32_t data)
{
#ifdef SUPPORT_MEMORYEVENT
	RaiseEvent(MemoryModel::MemEventWrite, addr, 4);
#endif

	*(uint32_t *)((uintptr_t)mem_base + addr.Get()) = data;
	return 0;
}

uint32_t ContiguousMemoryModel::PerformTranslation(Address virt_addr, Address &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info)
{
	out_phys_addr = virt_addr;
	return 0;
}

bool ContiguousMemoryModel::MapRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot, std::string name)
{
	return RegionBasedMemoryModel::MapRegion(addr, size, prot, name);
}

guest_addr_t ContiguousMemoryModel::MapAnonymousRegion(guest_size_t size, RegionFlags prot)
{
	return RegionBasedMemoryModel::MapAnonymousRegion(size, prot);
}
