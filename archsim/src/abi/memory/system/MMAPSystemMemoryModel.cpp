/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/memory/system/MMAPSystemMemoryModel.h"
#include "abi/devices/MMU.h"
#include "system.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "gensim/gensim_processor.h"

#include <sys/mman.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

using namespace archsim::abi::memory;

RegisterComponent(MemoryModel, MMAPPhysicalMemory, "mmap", "");

UseLogContext(LogMemoryModel);
DeclareChildLogContext(LogMMAP, LogMemoryModel, "MMAP");

struct {
	bool state;
} mmap_mem_state;

MMAPPhysicalMemory::MMAPPhysicalMemory()
{
}

MMAPPhysicalMemory::~MMAPPhysicalMemory()
{

}

bool MMAPPhysicalMemory::Initialise()
{
	int rc;

	physmem_fd = open("/home/spink/physmem", O_RDWR);
	if (physmem_fd < 0) {
		LC_ERROR(LogMMAP) << "Unable to open physical memory backing file";
		return false;
	}

	return true;
}

void MMAPPhysicalMemory::Destroy()
{
	close(physmem_fd);
}

MemoryTranslationModel &MMAPPhysicalMemory::GetTranslationModel()
{
	assert(false);
	__builtin_unreachable();
}

uint32_t MMAPPhysicalMemory::Read(guest_addr_t addr, uint8_t *data, int size)
{
	lseek(physmem_fd, addr, SEEK_SET);
	read(physmem_fd, data, size);
	return 0;
}

uint32_t MMAPPhysicalMemory::Fetch(guest_addr_t addr, uint8_t *data, int size)
{
	lseek(physmem_fd, addr, SEEK_SET);
	read(physmem_fd, data, size);
	return 0;
}

uint32_t MMAPPhysicalMemory::Write(guest_addr_t addr, uint8_t *data, int size)
{
	printf("MMapPhysicalMemory::Write addr: %x, data: %x\n", addr, *data);
	fflush(stdout);
	lseek(physmem_fd, addr, SEEK_SET);
	write(physmem_fd, data, size);
	fsync(physmem_fd);
	return 0;
}

uint32_t MMAPPhysicalMemory::Peek(guest_addr_t addr, uint8_t *data, int size)
{
	lseek(physmem_fd, addr, SEEK_SET);
	read(physmem_fd, data, size);
	return 0;
}

uint32_t MMAPPhysicalMemory::Poke(guest_addr_t addr, uint8_t *data, int size)
{
	lseek(physmem_fd, addr, SEEK_SET);
	write(physmem_fd, data, size);
	fsync(physmem_fd);
	return 0;
}

bool MMAPPhysicalMemory::ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr)
{
	return false;
}

MMAPVirtualMemory::MMAPVirtualMemory(System& system, MMAPPhysicalMemory& physmem) : system(system), physmem(physmem)
{
}

MMAPVirtualMemory::~MMAPVirtualMemory()
{

}

static bool HandleMMAPSegFault(void *ctx, const System::segfault_data& data)
{
	MMAPVirtualMemory *vmem = (MMAPVirtualMemory *)ctx;
	guest_addr_t virt_addr;
	void *r;

	if (!vmem->ResolveGuestAddress((host_const_addr_t)data.addr, virt_addr))
		return false;

	uint64_t host_page_base = ((uint64_t)data.addr) & ~4095ULL;
	uint32_t virtual_page_base = ((uint32_t)virt_addr) & ~4095;
	uint32_t phys_page_base;

	if (vmem->GetMMU().Translate(&vmem->GetCPU(), virtual_page_base, phys_page_base, MMUACCESSINFO(vmem->GetCPU().in_kernel_mode(), !mmap_mem_state.state), false)) {
		mmap_mem_state.state = false;
		return true;
	}

	r = mmap((void *)host_page_base, 4096, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, vmem->GetPhysicalMemory().GetPhysMemFD(), phys_page_base);
	if (r == MAP_FAILED)
		return false;

	msync((void *)host_page_base, 4096, MS_SYNC | MS_INVALIDATE);

	mmap_mem_state.state = true;
	return true;
}

bool MMAPVirtualMemory::Initialise()
{
	contiguous_size = 0x100000000;
	contiguous_base = (uint8_t *)mmap(NULL, contiguous_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
	if (contiguous_base == MAP_FAILED)
		return false;

	system.RegisterSegFaultHandler((uint64_t)contiguous_base, contiguous_size, this, HandleMMAPSegFault);
	return true;
}

void MMAPVirtualMemory::Destroy()
{
}

MemoryTranslationModel& MMAPVirtualMemory::GetTranslationModel()
{
	assert(false);
	__builtin_unreachable();
}

bool MMAPVirtualMemory::ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t& guest_addr)
{
	if ((uint8_t *)host_addr >= contiguous_base && (uint8_t *)host_addr < contiguous_base + contiguous_size) {
		guest_addr = (guest_addr_t)((uint8_t *)host_addr - contiguous_base);
		return true;
	}

	return false;
}

uint32_t MMAPVirtualMemory::Read(guest_addr_t addr, uint8_t* data, int size)
{
	mmap_mem_state.state = true;
	memcpy(data, contiguous_base + (uint32_t)addr, size);
	return !mmap_mem_state.state;
}

uint32_t MMAPVirtualMemory::Write(guest_addr_t addr, uint8_t* data, int size)
{
	mmap_mem_state.state = false;
	memcpy(contiguous_base + (uint32_t)addr, data, size);
	return !mmap_mem_state.state;
}

uint32_t MMAPVirtualMemory::Fetch(guest_addr_t addr, uint8_t* data, int size)
{
	mmap_mem_state.state = true;
	memcpy(data, contiguous_base + (uint32_t)addr, size);
	return !mmap_mem_state.state;
}

uint32_t MMAPVirtualMemory::Peek(guest_addr_t addr, uint8_t* data, int size)
{
	mmap_mem_state.state = true;
	memcpy(data, contiguous_base + (uint32_t)addr, size);
	return !mmap_mem_state.state;
}

uint32_t MMAPVirtualMemory::Poke(guest_addr_t addr, uint8_t* data, int size)
{
	mmap_mem_state.state = false;
	memcpy(contiguous_base + (uint32_t)addr, data, size);
	return !mmap_mem_state.state;
}

void MMAPVirtualMemory::FlushCaches()
{
	mprotect(contiguous_base, contiguous_size, PROT_NONE);
	msync(contiguous_base, contiguous_size, MS_SYNC | MS_INVALIDATE);
}

void MMAPVirtualMemory::EvictCacheEntry(virt_addr_t virt_addr)
{
	FlushCaches();
}
