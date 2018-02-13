/*
 * File:   ArmSystemEmulationModel.h
 * Author: harry
 *
 * Created on 16 December 2013, 11:02
 */

#ifndef ARMSYSTEMEMULATIONMODEL_H
#define	ARMSYSTEMEMULATIONMODEL_H

#include "abi/LinuxSystemEmulationModel.h"
#include "abi/devices/generic/block/FileBackedBlockDevice.h"

namespace archsim
{
	namespace arch
	{
		namespace arm
		{
			class ArmSystemEmulationModel : public archsim::abi::LinuxSystemEmulationModel
			{
			public:
				ArmSystemEmulationModel();
				~ArmSystemEmulationModel();

				bool Initialise(System& system, uarch::uArch& uarch) override;
				void Destroy() override;

				archsim::abi::ExceptionAction HandleException(gensim::Processor& cpu, uint32_t category, uint32_t data);

			protected:
				bool InstallDevices() override;
				void DestroyDevices() override;
				bool InstallPlatform(abi::loader::BinaryLoader& loader) override;

				bool PrepareCore(gensim::Processor& core) override;

			private:
				uint32_t entry_point;

				bool InstallPeripheralDevices();
				bool InstallPlatformDevices();
				bool InstallBootloader(archsim::abi::loader::BinaryLoader& loader);

				bool HackyMMIORegisterDevice(abi::devices::MemoryComponent& device);
				bool AddGenericPrimecellDevice(uint32_t base_addr, uint32_t size, uint32_t peripheral_id);

				void HandleSemihostingCall();

			};
		}
	}
}

#endif	/* ARMSYSTEMEMULATIONMODEL_H */

