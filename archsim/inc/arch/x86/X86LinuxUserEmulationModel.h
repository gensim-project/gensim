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
		namespace x86
		{
			class X86LinuxUserEmulationModel : public archsim::abi::LinuxUserEmulationModel
			{
			public:
				X86LinuxUserEmulationModel();
				~X86LinuxUserEmulationModel();

				bool Initialise(System& system, uarch::uArch& uarch) override;
				void Destroy() override;

				archsim::abi::ExceptionAction HandleException(archsim::core::thread::ThreadInstance *cpu, unsigned int category, unsigned int data);
				bool InvokeSignal(int signum, uint32_t next_pc, archsim::abi::SignalData* data);

				bool PrepareBoot(System& system);

				gensim::DecodeContext* GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu) override;


			private:
				Address vdso_ptr_;
			};
		}
	}
}

#endif	/* ARMLINUXUSEREMULATIONMODEL_H */

