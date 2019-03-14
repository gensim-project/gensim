/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   MemoryInterface.h
 * Author: harry
 *
 * Created on 10 April 2018, 16:10
 */

#ifndef MEMORYINTERFACE_H
#define MEMORYINTERFACE_H

#include "core/arch/ArchDescriptor.h"
#include "core/MemoryMonitor.h"
#include "abi/Address.h"
#include "abi/memory/MemoryModel.h"
#include "abi/devices/MMU.h"

namespace archsim
{

	enum class MemoryResult {
		OK,
		Error
	};

	enum class TranslationResult {
		UNKNOWN,
		OK,
		NotPresent,
		NotPrivileged
	};

	class MemoryTranslationProvider
	{
	public:
		virtual TranslationResult Translate(Address virt_addr, Address &phys_addr, bool is_write, bool is_fetch, bool side_effects) = 0;
	};

	class LamdaMemoryTranslationProvider : public MemoryTranslationProvider
	{
	public:
		using FnType = std::function<TranslationResult (Address, Address&, bool)>;

		LamdaMemoryTranslationProvider(FnType fn) : function_(fn) { }
		virtual ~LamdaMemoryTranslationProvider() { }
		TranslationResult Translate(Address virt_addr, Address& phys_addr, bool is_write, bool is_fetch, bool side_effects) override
		{
			return function_(virt_addr, phys_addr, side_effects);
		}

	private:
		FnType function_;
	};

	class IdentityTranslationProvider : public MemoryTranslationProvider
	{
	public:
		virtual ~IdentityTranslationProvider()
		{

		}
		TranslationResult Translate(Address virt_addr, Address& phys_addr, bool is_write, bool is_fetch, bool side_effects) override
		{
			phys_addr = virt_addr;
			return TranslationResult::OK;
		}

	};

	class MMUTranslationProvider : public MemoryTranslationProvider
	{
	public:
		MMUTranslationProvider(archsim::abi::devices::MMU *mmu, archsim::core::thread::ThreadInstance *thread) : mmu_(mmu), thread_(thread) {}
		virtual ~MMUTranslationProvider() {}

		TranslationResult Translate(Address virt_addr, Address& phys_addr, bool is_write, bool is_fetch, bool side_effects) override;
	private:
		archsim::abi::devices::MMU *mmu_;
		archsim::core::thread::ThreadInstance *thread_;
	};

	class MemoryDevice
	{
	public:
		virtual ~MemoryDevice();

		virtual MemoryResult Read8(Address address, uint8_t &data) = 0;
		virtual MemoryResult Read16(Address address, uint16_t &data) = 0;
		virtual MemoryResult Read32(Address address, uint32_t &data) = 0;
		virtual MemoryResult Read64(Address address, uint64_t &data) = 0;
		virtual MemoryResult Read128(Address address, uint128_t &data) = 0;

		virtual MemoryResult Write8(Address address, uint8_t data) = 0;
		virtual MemoryResult Write16(Address address, uint16_t data) = 0;
		virtual MemoryResult Write32(Address address, uint32_t data) = 0;
		virtual MemoryResult Write64(Address address, uint64_t data) = 0;
		virtual MemoryResult Write128(Address address, uint128_t data) = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	class LegacyMemoryInterface : public MemoryDevice
	{
	public:
		LegacyMemoryInterface(archsim::abi::memory::MemoryModel &mem_model) : mem_model_(mem_model) {}

		MemoryResult Read8(Address address, uint8_t& data) override;
		MemoryResult Read16(Address address, uint16_t& data) override;
		MemoryResult Read32(Address address, uint32_t& data) override;
		MemoryResult Read64(Address address, uint64_t& data) override;
		MemoryResult Read128(Address address, uint128_t& data) override;
		MemoryResult Write8(Address address, uint8_t data) override;
		MemoryResult Write16(Address address, uint16_t data) override;
		MemoryResult Write32(Address address, uint32_t data) override;
		MemoryResult Write64(Address address, uint64_t data) override;
		MemoryResult Write128(Address address, uint128_t data) override;

		void Lock() override
		{
			mem_model_.Lock();
		}
		void Unlock() override
		{
			mem_model_.Unlock();
		}

	private:
		archsim::abi::memory::MemoryModel &mem_model_;
	};

	class CachedLegacyMemoryInterface : public MemoryDevice
	{
	public:
		struct CacheEntry {
			Address tag;
			void *data;
		} __attribute__((packed));

		struct Cache {
			static const uint32_t kCacheSize = 1024;

			CacheEntry cache[kCacheSize];
		};

		CachedLegacyMemoryInterface(int index, archsim::abi::memory::MemoryModel &mem_model, archsim::core::thread::ThreadInstance *thread);

		MemoryResult Read8(Address address, uint8_t& data) override;
		MemoryResult Read16(Address address, uint16_t& data) override;
		MemoryResult Read32(Address address, uint32_t& data) override;
		MemoryResult Read64(Address address, uint64_t& data) override;
		MemoryResult Read128(Address address, uint128_t& data) override;
		MemoryResult Write8(Address address, uint8_t data) override;
		MemoryResult Write16(Address address, uint16_t data) override;
		MemoryResult Write32(Address address, uint32_t data) override;
		MemoryResult Write64(Address address, uint64_t data) override;
		MemoryResult Write128(Address address, uint128_t data) override;

		void Lock() override
		{
			mem_model_.Lock();
		}
		void Unlock() override
		{
			mem_model_.Unlock();
		}

		void Invalidate();

		MemoryResult Read(Address addr, char *data, size_t len);
		MemoryResult Write(Address addr, const char *data, size_t len);

	private:
		void *GetPtr(Address addr);
		void LoadEntryFor(struct CacheEntry *entry, Address addr);

		Cache *GetCache();

		archsim::abi::memory::MemoryModel &mem_model_;
		archsim::core::thread::ThreadInstance *thread_;
		uint64_t cache_offset_;
		std::mutex cache_lock_;
	};

	class LegacyFetchMemoryInterface : public MemoryDevice
	{
	public:
		LegacyFetchMemoryInterface(archsim::abi::memory::MemoryModel &mem_model) : mem_model_(mem_model) {}

		MemoryResult Read8(Address address, uint8_t& data) override;
		MemoryResult Read16(Address address, uint16_t& data) override;
		MemoryResult Read32(Address address, uint32_t& data) override;
		MemoryResult Read64(Address address, uint64_t& data) override;
		MemoryResult Read128(Address address, uint128_t& data) override;
		MemoryResult Write8(Address address, uint8_t data) override;
		MemoryResult Write16(Address address, uint16_t data) override;
		MemoryResult Write32(Address address, uint32_t data) override;
		MemoryResult Write64(Address address, uint64_t data) override;
		MemoryResult Write128(Address address, uint128_t data) override;

		void Lock() override
		{
			// do nothing
		}
		void Unlock() override
		{
			// do nothing
		}

	private:
		archsim::abi::memory::MemoryModel &mem_model_;
	};

	/**
	 * This class represents a specific instantiation of a memory master.
	 * This is connected to an underlying device (which may be a bus).
	 */
	class MemoryInterface
	{
	public:
		MemoryInterface(const MemoryInterfaceDescriptor &descriptor) : descriptor_(descriptor) {}
		const MemoryInterfaceDescriptor &GetDescriptor()
		{
			return descriptor_;
		}

		MemoryResult Read8 (Address address, uint8_t &data)
		{
			return device_->Read8(address, data);
		}
		MemoryResult Read16(Address address, uint16_t &data)
		{
			return device_->Read16(address, data);
		}
		MemoryResult Read32(Address address, uint32_t &data)
		{
			return device_->Read32(address, data);
		}
		MemoryResult Read64(Address address, uint64_t &data)
		{
			return device_->Read64(address, data);
		}
		MemoryResult Read128(Address address, uint128_t &data)
		{
			return device_->Read128(address, data);
		}

		MemoryResult Write8 (Address address, uint8_t data)
		{
			return device_->Write8(address, data);
		}
		MemoryResult Write16(Address address, uint16_t data)
		{
			return device_->Write16(address, data);
		}
		MemoryResult Write32(Address address, uint32_t data)
		{
			return device_->Write32(address, data);
		}
		MemoryResult Write64(Address address, uint64_t data)
		{
			return device_->Write64(address, data);
		}
		MemoryResult Write128(Address address, uint128_t data)
		{
			return device_->Write128(address, data);
		}

		MemoryResult ReadString(Address address,  char *data, size_t max_size);
		MemoryResult WriteString(Address address, const char *data);

		MemoryResult Read(Address address, unsigned char *data, size_t size);
		MemoryResult Write(Address address, const unsigned char *data, size_t size);

		TranslationResult PerformTranslation(Address virtual_address, Address &physical_address, bool is_write, bool is_fetch, bool side_effects)
		{
			return provider_->Translate(virtual_address, physical_address, is_write, is_fetch, side_effects);
		}

		void Connect(MemoryDevice &device)
		{
			device_ = &device;
		}
		void ConnectTranslationProvider(MemoryTranslationProvider &provider)
		{
			provider_ = &provider;
		}

		void Lock()
		{
			device_->Lock();
		}
		void Unlock()
		{
			device_->Unlock();
		}

		std::shared_ptr<archsim::core::MemoryMonitor> GetMonitor()
		{
			return monitor_;
		}
		void SetMonitor(std::shared_ptr<archsim::core::MemoryMonitor> new_monitor)
		{
			monitor_ = new_monitor;
		}

	private:
		const MemoryInterfaceDescriptor &descriptor_;
		MemoryTranslationProvider *provider_;
		MemoryDevice *device_;
		std::shared_ptr<archsim::core::MemoryMonitor> monitor_;
	};

}

#endif /* MEMORYINTERFACE_H */

