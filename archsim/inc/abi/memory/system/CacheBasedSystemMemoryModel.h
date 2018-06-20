/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * SystemMemoryModel.h
 *
 *  Created on: 9 Jul 2014
 *      Author: harry
 */

#ifndef CACHEBASEDSYSTEMMEMORYMODEL_H_
#define CACHEBASEDSYSTEMMEMORYMODEL_H_

#include "abi/memory/system/SystemMemoryModel.h"
#include "translate/profile/RegionArch.h"
#include "util/PubSubSync.h"
#include "util/LogContext.h"

#include "abi/Address.h"

#include <sys/mman.h>

#define CONFIG_SMM_NO_CACHE_DEVICES

#ifndef CONFIG_SMM_CACHE_DEVICES
#ifndef CONFIG_SMM_NO_CACHE_DEVICES
#error CONFIG_SMM_CACHE_DEVICES or CONFIG_SMM_NO_CACHE_DEVICES must be defined!
#endif
#endif

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
			class MemoryComponent;
			class DeviceManager;
			class MMU;
		}
		namespace memory
		{
			class CacheBasedSystemMemoryTranslationModel;

			class SMMCacheEntry
			{
			public:
				static const guest_addr_t::underlying_t kInvalidTag = 0xffffffffffffffff;

				static const uint32_t kIsDevice = 1;
				static const uint32_t kPrivRead = 2;
				static const uint32_t kPrivWrite = 4;
				static const uint32_t kUserWrite = 8;

				typedef uint8_t flag_t;

				SMMCacheEntry(guest_addr_t tag, Address phys_addr, void* data,flag_t flags);
				SMMCacheEntry();

				bool IsValid() const
				{
					return virt_tag.Get() != kInvalidTag;
				}
				bool IsPageFor(guest_addr_t addr) const
				{
					return addr.PageBase() == virt_tag;
				}

				// Returns true if data points to a device instance
				bool IsDevice() const
				{
					return GetFlags() & kIsDevice;
				}

				// Returns true if this region requires privileged permissions to read
				bool IsPrivilegedRead() const
				{
					return GetFlags() & kPrivRead;
				}

				// Returns true if this region requires privileged permissions to write
				bool IsPrivilegedWrite() const
				{
					return GetFlags() & kPrivWrite;
				}

				abi::devices::MemoryComponent* GetDevice() const
				{
					assert(IsDevice());
					return (abi::devices::MemoryComponent*)data;
				}
				void *GetMemory() const
				{
					assert(!IsDevice());
					return data;
				}

				guest_addr_t GetTag() const
				{
					return Address(virt_tag);
				}
				Address GetPhysAddr() const
				{
					return phys_addr & kPhysAddrMask;
				}

				inline void Invalidate()
				{
					virt_tag = Address(kInvalidTag);
				}

				void Dump()
				{
					if(IsDevice())
						fprintf(stderr, "SMM Entry %08x (%01x) => %08x (Device %p)\n", GetTag(), GetFlags(), GetPhysAddr(), GetDevice());
					else
						fprintf(stderr, "SMM Entry %08x (%01x) => %08x (Memory %p)\n", GetTag(), GetFlags(), GetPhysAddr(), GetMemory());
				}

				static const Address::underlying_t kPhysAddrMask = ~0xf;
				static const Address::underlying_t kFlagsMask = 0xf;
			private:


				inline flag_t GetFlags() const
				{
					return phys_addr.Get() & kFlagsMask;
				}

				Address virt_tag;
				Address phys_addr;
				void* data;

				char padding[8];
			} __attribute__((packed));

			static_assert(sizeof(SMMCacheEntry) == 32, "SMM Cache must be 32 bytes!");

			class SMMCache
			{
			public:
				SMMCache();
				~SMMCache();

				inline bool TryGetEntry(guest_addr_t addr, SMMCacheEntry *&entry)
				{
					dirty = true;
					entry = &GetEntryFor(addr);
					return entry->IsPageFor(addr);
				}

				inline void *GetCachePtr()
				{
					pointertaken = true;
					return (void*)_cache;
				}

				void Flush();
				void Evict(guest_addr_t addr);

				static const uint32_t kCacheBits = 10;
				static const uint32_t kCacheSize = 1 << kCacheBits;

				void Dump()
				{
//		fprintf(stderr, "Read Cache\n");
//		for(unsigned int i = 0; i < kCacheSize; ++i) {
//			fprintf(stderr, "Index %u: ", i);
//			read_cache[i].Dump();
//		}
//		fprintf(stderr, "Write Cache\n");
//		for(unsigned int i = 0; i < kCacheSize; ++i) {
//			fprintf(stderr, "Index %u: ", i);
//			write_cache[i].Dump();
//		}
				}

			private:

				static const uint32_t kCachePageBits = 5;
				static const uint32_t kCachePageCount = 1 << kCachePageBits;
				static const uint32_t kCachePageSize = kCacheSize / kCachePageCount;

				SMMCacheEntry *_cache;

				bool dirty, pointertaken;
				std::bitset<32> _dirty_pages;

				SMMCacheEntry &GetEntryFor(guest_addr_t addr)
				{
					dirty = true;
					uint32_t entry_idx = addr.GetPageIndex() % kCacheSize;
					_dirty_pages.set(entry_idx >> kCachePageBits);
					return _cache[entry_idx];
				}
			};

			/**
			 * This memory model implements much of the memory related functionality for
			 * system simulation, for example memory address translations, device lookups, etc.
			 */
			class CacheBasedSystemMemoryModel : public SystemMemoryModel
			{
			public:
				friend class CacheBasedSystemMemoryTranslationModel;

				CacheBasedSystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub);
				~CacheBasedSystemMemoryModel();

				bool Initialise() override;
				void Destroy() override;

				void FlushCaches() override;
				void EvictCacheEntry(Address virt_addr) override;

				void Dump()
				{
//		cache.Dump();
				}

				MemoryTranslationModel &GetTranslationModel() override;

				void InstallCaches();

				uint32_t Read(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Fetch(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Write(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Peek(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Poke(guest_addr_t guest_addr, uint8_t *data, int size) override;

				uint32_t Read8User(guest_addr_t guest_addr, uint32_t&data) override;
				uint32_t Read32User(guest_addr_t guest_addr, uint32_t&data) override;
				uint32_t Write8User(guest_addr_t guest_addr, uint8_t data) override;
				uint32_t Write32User(guest_addr_t guest_addr, uint32_t data) override;

				bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr) override;

				uint32_t PerformTranslation(Address virt_addr, Address &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info) override;

			private:

				uint32_t DoRead(guest_addr_t virt_addr, uint8_t *data, int size, bool use_perms);

				inline bool TryGetReadUserCacheEntry(guest_addr_t addr, SMMCacheEntry *&entry)
				{
					return _read_user_cache.TryGetEntry(addr, entry);
				}
				inline bool TryGetReadKernelCacheEntry(guest_addr_t addr, SMMCacheEntry *&entry)
				{
					return _read_kernel_cache.TryGetEntry(addr, entry);
				}
				inline bool TryGetWriteUserCacheEntry(guest_addr_t addr, SMMCacheEntry *&entry)
				{
					return _write_user_cache.TryGetEntry(addr, entry);
				}
				inline bool TryGetWriteKernelCacheEntry(guest_addr_t addr, SMMCacheEntry *&entry)
				{
					return _write_kernel_cache.TryGetEntry(addr, entry);
				}

				uint32_t UpdateCacheEntry(guest_addr_t addr, SMMCacheEntry *entry, bool isWrite, bool allow_side_effects);

				SMMCache _read_user_cache;
				SMMCache _write_user_cache;
				SMMCache _read_kernel_cache;
				SMMCache _write_kernel_cache;

				archsim::PhysicalAddress _prev_fetch_page_phys;
				archsim::VirtualAddress _prev_fetch_page_virt;

				SystemMemoryTranslationModel *translation_model;
				archsim::util::PubSubscriber *subscriber;
			};

		}
	}
}


#endif /* SYSTEMMEMORYMODEL_H_ */
