/*
 * File:   ArmRealviewEmulationModel.h
 * Author: harry
 *
 * Created on 16 December 2013, 11:02
 */

#ifndef ARMVERSATILEEMULATIONMODEL_H
#define	ARMVERSATILEEMULATIONMODEL_H

#include "abi/LinuxSystemEmulationModel.h"
#include "abi/devices/generic/block/FileBackedBlockDevice.h"

namespace archsim
{
	namespace arch
	{
		namespace arm
		{
			class ArmVersatileEmulationModel : public archsim::abi::LinuxSystemEmulationModel
			{
			public:
				ArmVersatileEmulationModel();
				~ArmVersatileEmulationModel();

				bool Initialise(System& system, uarch::uArch& uarch) override;
				void Destroy() override;

				archsim::abi::ExceptionAction HandleException(archsim::ThreadInstance *thread, uint32_t category, uint32_t data);
				gensim::DecodeContext* GetNewDecodeContext(archsim::ThreadInstance& cpu) override;


			protected:
				bool InstallDevices() override;
				void DestroyDevices() override;
				bool InstallPlatform(abi::loader::BinaryLoader& loader) override;

				bool PrepareCore(archsim::ThreadInstance& core) override;

			private:
				uint32_t entry_point;

				bool InstallPeripheralDevices();
				bool InstallPlatformDevices();
				bool InstallBootloader(archsim::abi::loader::BinaryLoader& loader);

				bool HackyMMIORegisterDevice(abi::devices::MemoryComponent& device);
				bool AddGenericPrimecellDevice(Address base_addr, uint32_t size, uint32_t peripheral_id);

				void HandleSemihostingCall();

			};
		}
	}
}

#endif	/* ARMSYSTEMEMULATIONMODEL_H */

