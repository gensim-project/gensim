/*
 * File:   SystemEmulationModel.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 14:52
 */

#ifndef SYSTEMEMULATIONMODEL_H
#define	SYSTEMEMULATIONMODEL_H

#include "abi/EmulationModel.h"
#include "abi/devices/DeviceManager.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class MemoryComponent;
			class CoreComponent;
		}
		namespace loader
		{
			class BinaryLoader;
		}

		namespace memory
		{
			class SystemMemoryModel;
		}

		class SystemEmulationModel : public EmulationModel
		{
			friend class SystemMemoryModel;

		public:
			SystemEmulationModel();
			virtual ~SystemEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

			gensim::Processor* GetCore(int id);
			gensim::Processor* GetBootCore();

			void ResetCores();
			void HaltCores();

			bool PrepareBoot(System& system) override;

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance *cpu, uint32_t category, uint32_t data) = 0;

			
			void PrintStatistics(std::ostream& stream);

			devices::DeviceManager& GetDeviceManager()
			{
				return base_device_manager;
			}

		protected:
			archsim::core::thread::ThreadInstance *main_thread_;

			virtual bool InstallDevices() = 0;
			virtual void DestroyDevices() = 0;

			virtual bool InstallPlatform(loader::BinaryLoader& loader) = 0;
			virtual bool PrepareCore(archsim::core::thread::ThreadInstance& core) = 0;

			bool RegisterMemoryComponent(abi::devices::MemoryComponent& component);
			void RegisterCoreComponent(abi::devices::CoreComponent& component);

			devices::DeviceManager base_device_manager;

		private:
			uint32_t rootfs_size;

			bool InstallAtags(archsim::abi::memory::guest_addr_t base_address, std::string kernel_args);
			bool InstallStartupCode(unsigned int entry_point, archsim::abi::memory::guest_addr_t atags_loc, uint32_t device_id);
			bool InstallDeviceTree(std::string device_tree_file);
			bool InstallRootfs(std::string rootfs_file, uint32_t rootfs_start);
		};
	}
}

#endif	/* SYSTEMEMULATIONMODEL_H */

