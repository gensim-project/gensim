/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef ARMDOOMLINUXUSEREMULATIONMODEL_H
#define	ARMDOOMLINUXUSEREMULATIONMODEL_H

#include "arch/arm/ArmLinuxUserEmulationModel.h"

namespace archsim
{
	namespace arch
	{
		namespace arm
		{
			class ArmDoomLinuxUserEmulationModel : public archsim::arch::arm::ArmLinuxUserEmulationModel
			{
			public:

				bool Initialise(System& system, uarch::uArch& uarch) override;
			};
		}
	}
}

#endif	/* ARMLINUXUSEREMULATIONMODEL_H */

