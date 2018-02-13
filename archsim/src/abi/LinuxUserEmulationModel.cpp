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
