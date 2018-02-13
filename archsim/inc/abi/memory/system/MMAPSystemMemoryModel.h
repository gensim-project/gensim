/*
 * File:   MMAPSystemMemoryModel.h
 * Author: s0457958
 *
 * Created on 28 October 2014, 17:01
 */

#ifndef MMAPSYSTEMMEMORYMODEL_H
#define	MMAPSYSTEMMEMORYMODEL_H

#include "abi/memory/MemoryModel.h"

class System;

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class MMU;
		}

		namespace memory
		{
			class MMAPPhysicalMemory : public MemoryModel
			{
			public:
				MMAPPhysicalMemory();
				virtual ~MMAPPhysicalMemory();

				bool Initialise() override;
				void Destroy() override;

				MemoryTranslationModel &GetTranslationModel() override;

				uint32_t Read(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Fetch(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Write(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) override;

				bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr) override;

				int GetPhysMemFD() const
				{
					return physmem_fd;
				}

			private:
				int physmem_fd;
			};

			class MMAPVirtualMemory : public MemoryModel
			{
			public:
				MMAPVirtualMemory(System& system, MMAPPhysicalMemory& physmem);
				virtual ~MMAPVirtualMemory();

				bool Initialise() override;
				void Destroy() override;

				MemoryTranslationModel &GetTranslationModel() override;

				uint32_t Read(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Fetch(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Write(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) override;

				bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr) override;

				inline void SetCPU(gensim::Processor& cpu)
				{
					this->cpu = &cpu;
				}
				inline gensim::Processor& GetCPU() const
				{
					return *cpu;
				}
				inline void SetMMU(devices::MMU& mmu)
				{
					this->mmu = &mmu;
				}
				inline devices::MMU& GetMMU() const
				{
					return *mmu;
				}

				inline MMAPPhysicalMemory& GetPhysicalMemory() const
				{
					return physmem;
				}

				void FlushCaches();
				void EvictCacheEntry(virt_addr_t virt_addr);

			private:
				System& system;

				gensim::Processor *cpu;
				devices::MMU *mmu;

				MMAPPhysicalMemory& physmem;

				uint8_t *contiguous_base;
				size_t contiguous_size;
			};
		}
	}
}

#endif	/* MMAPSYSTEMMEMORYMODEL_H */

