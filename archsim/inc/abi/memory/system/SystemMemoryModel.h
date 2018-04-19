/*
 * SystemMemoryModel.h
 *
 *  Created on: 22 Apr 2015
 *      Author: harry
 */

#ifndef SYSTEMMEMORYMODEL_H_
#define SYSTEMMEMORYMODEL_H_

#include "abi/memory/MemoryModel.h"
#include "translate/profile/ProfileManager.h"
#include "util/PubSubSync.h"
#include "gensim/gensim_processor.h"

namespace archsim
{
	namespace abi
	{

		namespace devices
		{
			class DeviceManager;
			class MMU;
		}

		namespace memory
		{

			class SystemMemoryTranslationModel;

			class SystemMemoryModel : public MemoryModel
			{
			public:

				friend class SystemMemoryTranslationModel;

				SystemMemoryModel(MemoryModel *phys_mem, util::PubSubContext *pubsub) : phys_mem(phys_mem), mmu(NULL), thread_(NULL), dev_man(NULL) {}
				SystemMemoryModel() = delete;

				void SetMMU(devices::MMU *mmu)
				{
					this->mmu = mmu;
				}
				void SetDeviceManager(devices::DeviceManager *devman)
				{
					dev_man = devman;
				}
				void SetCPU(archsim::ThreadInstance *cpu)
				{
					this->thread_ = cpu;
				}

				void MarkPageAsCode(PhysicalAddress page_base)
				{
					GetProfile().MarkRegionAsCode(page_base);
				}

			protected:
				devices::MMU *GetMMU()
				{
					return mmu;
				}
				devices::DeviceManager *GetDeviceManager()
				{
					return dev_man;
				}
				archsim::ThreadInstance *GetThread()
				{
					return thread_;
				}
				MemoryModel *GetPhysMem()
				{
					return phys_mem;
				}
				translate::profile::ProfileManager &GetProfile()
				{
					return this->GetThread()->GetEmulationModel().GetSystem().GetProfileManager();
				}

			private:
				MemoryModel *phys_mem;
				devices::DeviceManager *dev_man;
				devices::MMU *mmu;
				archsim::ThreadInstance *thread_;

			};

		}
	}
}

#endif /* SYSTEMMEMORYMODEL_H_ */
