/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LinuxUserEmulationModel.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 14:49
 */

#ifndef LINUXUSEREMULATIONMODEL_H
#define	LINUXUSEREMULATIONMODEL_H

#include "abi/UserEmulationModel.h"

namespace archsim
{
	namespace abi
	{
		class LinuxUserEmulationModel : public UserEmulationModel
		{
		public:
			LinuxUserEmulationModel(const user::arch_descriptor_t &arch);
			virtual ~LinuxUserEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance* cpu, unsigned int category, unsigned int data) = 0;
		};
	}
}

#endif	/* LINUXUSEREMULATIONMODEL_H */

