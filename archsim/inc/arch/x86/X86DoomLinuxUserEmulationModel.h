/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef ARMDOOMLINUXUSEREMULATIONMODEL_H
#define	ARMDOOMLINUXUSEREMULATIONMODEL_H

#include "arch/x86/X86LinuxUserEmulationModel.h"

namespace archsim
{
	namespace arch
	{
		namespace x86
		{
			class X86DoomLinuxUserEmulationModel : public archsim::arch::x86::X86LinuxUserEmulationModel
			{
			public:

				bool Initialise(System& system, uarch::uArch& uarch) override;
			};
		}
	}
}

#endif	/* ARMLINUXUSEREMULATIONMODEL_H */

