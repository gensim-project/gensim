/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * BaseSystemMemoryModel.h
 *
 *  Created on: 22 Apr 2015
 *      Author: harry
 */

#ifndef NOCPUBASESYSTEMMEMORYMODEL_H_
#define NOCPUBASESYSTEMMEMORYMODEL_H_

#include "abi/memory/system/SystemMemoryModel.h"

namespace archsim
{
	namespace abi
	{
		namespace memory
		{

			class BaseSystemMemoryTranslationModel;

			class NoCPUBaseSystemMemoryModel : public SystemMemoryModel
			{
			public:
				friend class BaseSystemMemoryTranslationModel;

				NoCPUBaseSystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub);

				bool Initialise() override;
				void Destroy() override;

				MemoryTranslationModel &GetTranslationModel() override;

				uint32_t Read(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Fetch(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Write(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Peek(guest_addr_t guest_addr, uint8_t *data, int size) override;
				uint32_t Poke(guest_addr_t guest_addr, uint8_t *data, int size) override;

				uint32_t Read8User(guest_addr_t guest_addr, uint32_t&data) override;
				uint32_t Read32User(guest_addr_t guest_addr, uint32_t&data) override;
				uint32_t Write8User(guest_addr_t guest_addr, uint8_t data) override;
				uint32_t Write32User(guest_addr_t guest_addr, uint32_t data) override;

				bool ResolveGuestAddress(host_const_addr_t, guest_addr_t&) override;

				uint32_t PerformTranslation(virt_addr_t virt_addr, phys_addr_t &out_phys_addr, const struct archsim::abi::devices::AccessInfo &info) override;

				enum UnalignedBehaviour {
					Unaligned_TRAP,
					Unaligned_EMULATE
				};

				void SetUnalignedBehaviour(UnalignedBehaviour behaviour)
				{
					_unaligned_behaviour = behaviour;
				}

			private:
				uint32_t DoRead(guest_addr_t guest_addr, uint8_t *data, int size, bool use_perms, bool side_effects, bool is_fetch);
				uint32_t DoWrite(guest_addr_t guest_addr, uint8_t *data, int size, bool use_perms, bool side_effects);

				BaseSystemMemoryTranslationModel *txln_model;
				UnalignedBehaviour _unaligned_behaviour;
			};

		}
	}
}


#endif /* NOCPUBASESYSTEMMEMORYMODEL_H_ */
