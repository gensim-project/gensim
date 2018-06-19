/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/LinuxUserEmulationModel.h"

using namespace archsim::abi;

LinuxUserEmulationModel::LinuxUserEmulationModel(const user::arch_descriptor_t &arch) : UserEmulationModel(arch) { }

LinuxUserEmulationModel::~LinuxUserEmulationModel() { }

bool LinuxUserEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	return UserEmulationModel::Initialise(system, uarch);
}

void LinuxUserEmulationModel::Destroy()
{
	UserEmulationModel::Destroy();
}
