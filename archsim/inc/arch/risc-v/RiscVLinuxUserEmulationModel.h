/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
				RiscVLinuxUserEmulationModel(bool rv64);
				~RiscVLinuxUserEmulationModel();

				bool Initialise(System& system, uarch::uArch& uarch) override;
				void Destroy() override;

				archsim::abi::ExceptionAction HandleException(archsim::core::thread::ThreadInstance *cpu, uint64_t category, uint64_t data);
				bool InvokeSignal(int signum, uint32_t next_pc, archsim::abi::SignalData* data);

				bool PrepareBoot(System& system);

				gensim::DecodeContext* GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu) override;


			private:

			};
		}
	}
}

#endif	/* ARMLINUXUSEREMULATIONMODEL_H */

