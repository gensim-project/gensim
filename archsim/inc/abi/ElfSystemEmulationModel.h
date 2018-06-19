/*
 * ElfSystemEmulationModel.h
 *
 *  Created on: 8 Sep 2014
 *      Author: harry
 */

#ifndef ELFSYSTEMEMULATIONMODEL_H_
#define ELFSYSTEMEMULATIONMODEL_H_

#include "abi/SystemEmulationModel.h"

namespace archsim
{
	namespace abi
	{

		class ElfSystemEmulationModel : public SystemEmulationModel
		{
		public:
			ElfSystemEmulationModel();
			virtual ~ElfSystemEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance *thread, uint32_t category, uint32_t data) override;

		protected:

			virtual bool InstallDevices() override;
			virtual void DestroyDevices() override;

			virtual bool InstallPlatform(loader::BinaryLoader& loader) override;
			virtual bool PrepareCore(archsim::core::thread::ThreadInstance& core) override;

			bool HandleSemihostingCall();

		private:
			bool InstallBootloader(std::string filename);
			bool InstallKernelHelpers();
			bool PrepareStack();

			Address binary_entrypoint;
			Address initial_sp;

			Address heap_base, heap_limit, stack_base, stack_limit;
		};
	}
}


#endif /* ELFSYSTEMEMULATIONMODEL_H_ */
