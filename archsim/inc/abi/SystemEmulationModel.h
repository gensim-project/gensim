/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SystemEmulationModel.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 14:52
 */

#ifndef SYSTEMEMULATIONMODEL_H
#define	SYSTEMEMULATIONMODEL_H

#include "core/execution/ExecutionEngine.h"
#include "core/MemoryMonitor.h"
#include "abi/EmulationModel.h"
#include "abi/devices/DeviceManager.h"
#include "loader/BinaryLoader.h"

#include <memory>

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
			SystemEmulationModel(bool is_64bit);
			virtual ~SystemEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

			void HaltCores() override;

			bool PrepareBoot(System& system) override;

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance *cpu, uint64_t category, uint64_t data) override = 0;


			void PrintStatistics(std::ostream& stream) override;

			devices::DeviceManager& GetDeviceManager()
			{
				return base_device_manager;
			}

			bool Is64Bit() const
			{
				return is_64bit_;
			}

			archsim::core::thread::ThreadInstance &GetThread(int thread_id)
			{
				return *threads_.at(thread_id);
			}

		protected:
			std::vector<archsim::core::thread::ThreadInstance*> threads_;

			bool InstantiateThreads(int num_threads);

			virtual bool CreateCoreDevices(archsim::core::thread::ThreadInstance *thread) = 0;
			virtual bool CreateMemoryDevices() = 0;

			virtual bool PreparePlatform(archsim::abi::loader::BinaryLoader &loader) = 0;
			virtual bool PrepareCore(archsim::core::thread::ThreadInstance& core) = 0;

			bool RegisterMemoryComponent(abi::devices::MemoryComponent& component);

			devices::DeviceManager base_device_manager;

		private:
			bool is_64bit_;
			uint32_t rootfs_size;
			archsim::core::execution::ExecutionEngine *execution_engine_;
			std::shared_ptr<archsim::core::MemoryMonitor> monitor_;

			bool InstallAtags(archsim::abi::memory::guest_addr_t base_address, std::string kernel_args);
			bool InstallStartupCode(unsigned int entry_point, archsim::abi::memory::guest_addr_t atags_loc, uint32_t device_id);
			bool InstallDeviceTree(std::string device_tree_file);
			bool InstallRootfs(std::string rootfs_file, uint32_t rootfs_start);
		};
	}
}

#endif	/* SYSTEMEMULATIONMODEL_H */

