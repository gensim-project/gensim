/*
 * File:   FlipperMemoryModel.h
 * Author: s0457958
 *
 * Created on 13 December 2013, 13:43
 */

#ifndef FLIPPERMEMORYMODEL_H
#define	FLIPPERMEMORYMODEL_H

#include "abi/memory/MemoryModel.h"

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class MemoryModel;
			class MemoryTranslationModel;

			class FlipperMemoryModel : public MemoryModel, public MappingManager
			{
			public:
				FlipperMemoryModel(MemoryModel& lhs, MemoryModel& rhs, guest_addr_t split_address);
				~FlipperMemoryModel();

				bool Initialise();
				MappingManager *GetMappingManager() override;

				bool MapAll(RegionFlags prot) override;
				bool MapRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot, std::string name) override;
				bool RemapRegion(guest_addr_t addr, guest_size_t size) override;
				bool UnmapRegion(guest_addr_t addr, guest_size_t size) override;
				bool ProtectRegion(guest_addr_t addr, guest_size_t size, RegionFlags prot) override;
				bool GetRegionProtection(guest_addr_t addr, RegionFlags& prot) override;
				guest_addr_t MapAnonymousRegion(guest_size_t size, RegionFlags prot) override;
				void DumpRegions() override;

				bool ResolveGuestAddress(host_const_addr_t host_addr, guest_addr_t &guest_addr);
				bool HandleSegFault(host_const_addr_t host_addr);

				virtual uint32_t Read(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Write(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) override;

				MemoryTranslationModel &GetTranslationModel();

			private:
				MemoryTranslationModel *translation_model;

				const guest_addr_t split_address;

			public:
				MemoryModel& lhs;
				MemoryModel& rhs;
				MappingManager *lhs_map;
				MappingManager *rhs_map;
			};
		}
	}
}

#endif	/* FLIPPERMEMORYMODEL_H */

