/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <abi/devices/Component.h>
#include <module/Module.h>
#include <cstdint>

#include <cstddef>

#include "GenericInterruptController.h"
#include "GenericPrimecellDevice.h"
#include "VersatileSIC.h"
#include "PL011.h"
#include "PL031.h"
#include "PL050.h"
#include "PL061.h"
#include "PL080.h"
#include "PL110.h"
#include "PL180.h"
#include "PL190.h"
#include "SP804.h"
#include "SP810.h"

using namespace archsim::abi::external::arm;
using namespace archsim::module;

ARCHSIM_MODULE()
{
	auto module = new ModuleInfo("devices.arm.system", "Arm system-level devices");

	module->AddEntry(new ModuleComponentEntry("GIC.GIC", ARCHSIM_COMPONENTFACTORY(GIC)));
	module->AddEntry(new ModuleDeviceEntry("GIC.CpuInterface", ARCHSIM_DEVICEFACTORY(GICCPUInterface)));
	module->AddEntry(new ModuleDeviceEntry("GIC.Distributor", ARCHSIM_DEVICEFACTORY(GICDistributorInterface)));

	module->AddEntry(new ModuleDeviceEntry("GenericPrimecellDevice", ARCHSIM_DEVICEFACTORY(GenericPrimecellDevice)));

	module->AddEntry(new ModuleDeviceEntry("VersatileSIC", ARCHSIM_DEVICEFACTORY(VersatileSIC<>)));

	module->AddEntry(new ModuleDeviceEntry("PL011", ARCHSIM_DEVICEFACTORY(PL011)));
	module->AddEntry(new ModuleDeviceEntry("PL031", ARCHSIM_DEVICEFACTORY(PL031)));
	module->AddEntry(new ModuleDeviceEntry("PL050", ARCHSIM_DEVICEFACTORY(PL050)));
	module->AddEntry(new ModuleDeviceEntry("PL061", ARCHSIM_DEVICEFACTORY(PL061)));
	module->AddEntry(new ModuleDeviceEntry("PL080", ARCHSIM_DEVICEFACTORY(PL080)));
	module->AddEntry(new ModuleDeviceEntry("PL110", ARCHSIM_DEVICEFACTORY(PL110)));
	module->AddEntry(new ModuleDeviceEntry("PL180", ARCHSIM_DEVICEFACTORY(PL180)));
	module->AddEntry(new ModuleDeviceEntry("PL190", ARCHSIM_DEVICEFACTORY(PL190)));
	module->AddEntry(new ModuleDeviceEntry("SP804", ARCHSIM_DEVICEFACTORY(SP804)));
	module->AddEntry(new ModuleDeviceEntry("SP810", ARCHSIM_DEVICEFACTORY(SP810)));

	return module;
}

ARCHSIM_MODULE_END()
{

}
