/*
 * File:   ArmLinuxUserEmulationModel.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 14:50
 */

#ifndef ARMLINUXUSEREMULATIONMODEL_H
#define	ARMLINUXUSEREMULATIONMODEL_H

#include "abi/LinuxUserEmulationModel.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{
			class RiscVLinuxUserEmulationModel : public archsim::abi::LinuxUserEmulationModel
			{
			public:
				RiscVLinuxUserEmulationModel();
				~RiscVLinuxUserEmulationModel();

				bool Initialise(System& system, uarch::uArch& uarch) override;
				void Destroy() override;

				archsim::abi::ExceptionAction HandleException(archsim::ThreadInstance *cpu, unsigned int category, unsigned int data);
				bool InvokeSignal(int signum, uint32_t next_pc, archsim::abi::SignalData* data);

				bool PrepareBoot(System& system);

				gensim::DecodeContext* GetNewDecodeContext(archsim::ThreadInstance& cpu) override;


			private:

			};
		}
	}
}

#endif	/* ARMLINUXUSEREMULATIONMODEL_H */

