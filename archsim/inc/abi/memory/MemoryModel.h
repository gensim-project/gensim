/*
 * File:   MemoryModel.h
 * Author: s0457958
 *
 * Created on 14 November 2013, 15:50
 */

#ifndef MEMORYMODEL_H
#define MEMORYMODEL_H

#include "define.h"

#include "translate/profile/ProfileManager.h"
#include "abi/devices/Component.h"

#include <string.h>
#include <string>
#include <map>
#include <vector>

#ifndef CONFIG_NO_MEMORY_EVENTS
#ifndef CONFIG_MEMORY_EVENTS
#error Either CONFIG_NO_MEMORY_EVENTS or CONFIG_MEMORY_EVENTS must be defined!
#endif
#endif

using namespace archsim::abi::devices;

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace abi
	{
		class SystemEmulationModel;

		namespace devices
		{
			struct AccessInfo;
		}

		namespace memory
		{

			enum RegionLockType {
				LockRead = 1,
				LockWrite = 2,
				LockExecute = 4,
				LockBypassProtection = 8,
			};

			enum RegionFlags {
				RegFlagNone = 0,
				RegFlagRead = 1,
				RegFlagWrite = 2,
				RegFlagReadWrite = 3,
				RegFlagExecute = 4,
				RegFlagReadExecute = 5,
				RegFlagWriteExecute = 6,
				RegFlagReadWriteExecute = 7,
				RegFlagDoNotCache = 8,
			};

			enum Endianness {
				LittleEndian,
				BigEndian,
			};

			typedef uint32_t guest_addr_t;
			typedef uint32_t guest_size_t;
			typedef const void *host_const_addr_t;

			class MemoryTranslationModel;

			struct MemoryUsageInfo {
				struct {
					uint64_t rss;
				} host;
				struct {
					uint64_t total;
				} guest;
			};

			class MappingManager
			{
			public:
				virtual ~MappingManager();

				virtual bool MapAll(RegionFlags prot) = 0;
				virtual bool MapRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot, std::string name) = 0;
				virtual bool RemapRegion(guest_addr_t addr, guest_size_t size) = 0;
				virtual bool UnmapRegion(guest_addr_t addr, guest_size_t size) = 0;
				virtual bool ProtectRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot) = 0;
				virtual bool GetRegionProtection(guest_addr_t addr, RegionFlags& prot) = 0;
				virtual guest_addr_t MapAnonymousRegion(guest_size_t size, RegionFlags prot) = 0;
				virtual void DumpRegions() = 0;
			};

			class MemoryEventHandler;

			/**
			 * Base memory model class. A memory model supports reading and writing, which may affect state,
			 * and 'peeking' and 'poking' which should read/modify memory without otherwise affecting the
			 * state of the system.
			 *
			 * Read/write 8/16/32 helper functions are provided - the default behaviour of these is to call
			 * read and write with appropriate arguments but they can be overridden for performance
			 *
			 * The memory model also exposes a translationmodel, which should be able to translate memory
			 * accesses to LLVM IR.
			 */
			class MemoryModel : public Component
			{
			public:
				enum MemoryEventType {
					MemEventRead,
					MemEventWrite,
					MemEventFetch
				};

				MemoryModel();
				virtual ~MemoryModel();

				virtual bool Initialise() = 0;
				virtual void Destroy() = 0;

				virtual bool GetMemoryUsage(MemoryUsageInfo& usage);

				virtual MemoryTranslationModel &GetTranslationModel() = 0;
				virtual MappingManager *GetMappingManager();

				/**
				 * Attempt to obtain a host pointer for the given region of guest memory. Returns TRUE if this has
				 * succeeded.
				 */
				virtual bool LockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t& host_addr);

				/**
				 * Unlock a region of memory previously locked by a LockRegion call.
				 */
				virtual bool UnlockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t host_addr);

				/**
				 * Flush any caches associated with this memory model.
				 */
				virtual void FlushCaches();

				/**
				 * Evict the specified cache entry from the cache
				* @param virt_addr
				*/
				virtual void EvictCacheEntry(virt_addr_t virt_addr);

				/**
				 * Perform a virtual->physical (oe equivalant) memory translation. This function
				 * returns 0 if the translation succeeded, or a non-zero error code if it failed.
				 * If this memory model does not support translation, it should set the physical
				 * address to the virtual address and return 0 (ie., perform an identity translation)
				 */
				virtual uint32_t PerformTranslation(virt_addr_t virt_addr, phys_addr_t &out_phys_addr, const struct abi::devices::AccessInfo &info);

				/**
				 * Loads and copies the given file into this memory model.
				* @param addr The guest address at which to place the file.
				* @param filename The filename of the file to insert into memory.
				* @return Returns TRUE if the operation succeeded, FALSE otherwise.
				*/
				virtual bool InsertFile(guest_addr_t addr, std::string filename, uint32_t& size);

				/**
				 * Read a number of bytes from the given guest adress. On success, 0 is returned. On failure,
				 * a non-zero system specific error code is returned.
				 */
				virtual uint32_t Read(guest_addr_t addr, uint8_t *data, int size) = 0;

				/**
				 * Fetch a number of bytes from the given guest adress. On success, 0 is returned. On failure,
				 * a non-zero system specific error code is returned.
				 */
				virtual uint32_t Fetch(guest_addr_t addr, uint8_t *data, int size) = 0;

				/**
				 * Write a number of bytes from the given guest adress. On success, 0 is returned. On failure,
				 * a non-zero system specific error code is returned.
				 */
				virtual uint32_t Write(guest_addr_t addr, uint8_t *data, int size) = 0;

				/**
				 * Read a number of bytes out of guest memory, without modifying any internal state. On success, 0
				 * is returned, on failure a non zero system specific error code is returned.
				 */
				virtual uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) = 0;

				/**
				 * Write a number of bytes into guest memory, without modifying any internal state. On success, 0
				 * is returned, on failure a non zero system specific error code is returned.
				 */
				virtual uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) = 0;

				virtual uint32_t Read8(guest_addr_t addr, uint8_t &data);
				virtual uint32_t Read16(guest_addr_t addr, uint16_t &data);
				virtual uint32_t Read32(guest_addr_t addr, uint32_t &data);
				virtual uint32_t Read64(guest_addr_t addr, uint64_t &data);

				virtual uint32_t Read8_zx(guest_addr_t addr, uint32_t &data);
				virtual uint32_t Read16_zx(guest_addr_t addr, uint32_t &data);
				virtual uint32_t Read8_sx(guest_addr_t addr, uint32_t &data);
				virtual uint32_t Read16_sx(guest_addr_t addr, uint32_t &data);

				virtual uint32_t Fetch8(guest_addr_t addr, uint8_t &data);
				virtual uint32_t Fetch16(guest_addr_t addr, uint16_t &data);
				virtual uint32_t Fetch32(guest_addr_t addr, uint32_t &data);

				virtual uint32_t Write8(guest_addr_t addr, uint8_t data);
				virtual uint32_t Write16(guest_addr_t addr, uint16_t data);
				virtual uint32_t Write32(guest_addr_t addr, uint32_t data);
				virtual uint32_t Write64(guest_addr_t addr, uint64_t data);

				virtual uint32_t Read8User(guest_addr_t addr, uint32_t &data);
				virtual uint32_t Read32User(guest_addr_t addr, uint32_t &data);
				virtual uint32_t Write8User(guest_addr_t addr, uint8_t data);
				virtual uint32_t Write32User(guest_addr_t addr, uint32_t data);

				virtual uint32_t ReadString(guest_addr_t addr, char *str, int size);
				virtual uint32_t WriteString(guest_addr_t addr, const char *str);

				virtual uint32_t ReadN(guest_addr_t addr, uint8_t *buffer, size_t size);
				virtual uint32_t WriteN(guest_addr_t addr, uint8_t *buffer, size_t size);

				virtual uint32_t Peek32(guest_addr_t addr, uint32_t &data);

				uint32_t Peek32Unsafe(guest_addr_t addr);

				virtual bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr) = 0;
				virtual bool HandleSegFault(host_const_addr_t host_addr);

				inline guest_addr_t AlignDown(guest_addr_t addr) const __attribute__((pure))
				{
					return addr & ~4095;
				}

				inline guest_addr_t AlignUp(guest_addr_t addr) const __attribute__((pure))
				{
					if ((addr & ~4095) == addr) return addr;
					return AlignDown(addr) + 4096;
				}

				inline bool IsAligned(guest_addr_t addr) const __attribute__((pure))
				{
					return (addr & ~4095) == addr;
				}

				inline void RegisterEventHandler(MemoryEventHandler& handler)
				{
					event_handlers.push_back(&handler);
				}

				inline bool HasEventHandlers() const
				{
					return event_handlers.size() > 0;
				}

				inline const std::vector<MemoryEventHandler *>& GetEventHandlers() const
				{
					return event_handlers;
				};

				bool RaiseEvent(MemoryEventType type, guest_addr_t addr, uint8_t size);

				inline void SetProcessor(gensim::Processor& cpu)
				{
					this->cpu = &cpu;
				}

			private:
				gensim::Processor *cpu;
				std::vector<MemoryEventHandler *> event_handlers;
			};

			/**
			 * Null memory model which simply returns an error code whenever accesses are attempted
			 */
			class NullMemoryModel : public MemoryModel
			{
			public:
				NullMemoryModel();
				~NullMemoryModel();

				bool Initialise() override;
				void Destroy() override;

				virtual uint32_t Read(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Fetch(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Write(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) override;

				virtual bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr);
				virtual bool HandleSegFault(host_const_addr_t host_addr);

				virtual MemoryTranslationModel &GetTranslationModel();
				virtual uint32_t PerformTranslation(virt_addr_t virt_addr, phys_addr_t &out_phys_addr, const struct abi::devices::AccessInfo &info);

			private:
				MemoryTranslationModel *translation_model;
			};

			struct GuestVMA {
				host_addr_t host_base;
				guest_addr_t base;
				guest_size_t size;
				RegionFlags protection;
				std::string name;
			};

			class RegionBasedMemoryModel : public MemoryModel, public MappingManager
			{
			public:
				RegionBasedMemoryModel();
				virtual ~RegionBasedMemoryModel();

				bool Initialise() = 0;
				void Destroy() = 0;

				bool MapAll(RegionFlags prot) override;
				bool MapRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot, std::string name) override;
				guest_addr_t MapAnonymousRegion(guest_size_t size, RegionFlags prot) override;
				bool RemapRegion(guest_addr_t addr, guest_size_t size) override;
				bool UnmapRegion(guest_addr_t addr, guest_size_t size) override;
				bool ProtectRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot) override;
				bool GetRegionProtection(guest_addr_t addr, RegionFlags& prot) override;
				void DumpRegions() override;

				virtual bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr) override;
				virtual bool GetMemoryUsage(MemoryUsageInfo& usage) override;

				virtual MappingManager *GetMappingManager() override;

			protected:
				bool HasIntersectingRegions(guest_addr_t addr, guest_size_t size);
				bool VMAIntersects(GuestVMA& vma, guest_size_t size);
				bool DeleteVMA(guest_addr_t addr);
				GuestVMA *LookupVMA(guest_addr_t addr);

				virtual bool AllocateVMA(GuestVMA &vma) = 0;
				virtual bool DeallocateVMA(GuestVMA &vma) = 0;
				virtual bool ResizeVMA(GuestVMA &vma, guest_size_t new_size) = 0;
				virtual bool SynchroniseVMAProtection(GuestVMA &vma) = 0;

				inline GuestVMA *TryCachedVMALookup(guest_addr_t addr) const
				{
					bool result = ((addr >= cached_vma->base) && (addr < (cached_vma->base+cached_vma->size)));
					return result ? cached_vma : NULL;
				}

				typedef std::map<guest_addr_t, GuestVMA *> GuestVMAMap;

			private:
				GuestVMAMap guest_vmas;
				GuestVMA *cached_vma;
			};

			class ContiguousMemoryModel : public RegionBasedMemoryModel
			{
			public:
				ContiguousMemoryModel();
				~ContiguousMemoryModel();

				bool Initialise() override;
				void Destroy() override;

				bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr);

				bool LockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t& host_addr);
				bool UnlockRegion(guest_addr_t guest_addr, guest_size_t guest_size, host_addr_t host_addr);

				uint32_t Read(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Fetch(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Write(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) override;
				uint32_t Read8(guest_addr_t addr, uint8_t &data) override;
				uint32_t Read16(guest_addr_t addr, uint16_t &data) override;
				uint32_t Read32(guest_addr_t addr, uint32_t &data) override;
				uint32_t Fetch8(guest_addr_t addr, uint8_t &data) override;
				uint32_t Fetch16(guest_addr_t addr, uint16_t &data) override;
				uint32_t Fetch32(guest_addr_t addr, uint32_t &data) override;
				uint32_t Write8(guest_addr_t addr, uint8_t data) override;
				uint32_t Write16(guest_addr_t addr, uint16_t data) override;
				uint32_t Write32(guest_addr_t addr, uint32_t data) override;

				virtual uint32_t PerformTranslation(virt_addr_t virt_addr, phys_addr_t &out_phys_addr, const struct abi::devices::AccessInfo &info) override;

				MemoryTranslationModel &GetTranslationModel() override;
				host_addr_t mem_base;

			protected:
				bool AllocateVMA(GuestVMA &vma);
				bool DeallocateVMA(GuestVMA &vma);
				bool ResizeVMA(GuestVMA &vma, guest_size_t new_size);
				bool SynchroniseVMAProtection(GuestVMA &vma);

			private:
				MemoryTranslationModel *translation_model;

				bool is_initialised;

				inline host_addr_t GuestToHost(guest_addr_t addr) const
				{
					return (host_addr_t)((unsigned long)mem_base + (unsigned long)addr);
				}
			};
		}
	}
}
#endif /* MEMORYMODEL_H */
