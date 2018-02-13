/*
 * abi/devices/arm/external/PL180.cpp
 */
#include "PL180.h"
#include "abi/devices/IRQController.h"
#include "util/LogContext.h"

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL180, LogArmExternalDevice, "PL180");

using namespace archsim::abi::devices;
using namespace archsim::abi::external::arm;

static ComponentDescriptor pl180_descriptor ("PL180", {
	{"mci0a_pri", ComponentParameter_Component},
	{"mci0a_sec", ComponentParameter_Component},
	{"mci0b_sec", ComponentParameter_Component}
});
PL180::PL180(EmulationModel &parent, Address base_address)
	: PrimecellRegisterDevice(parent, base_address, 0x1000, 0x00041180, 0xb105f00d, "pl180"),
	  Component(pl180_descriptor),
	  MCIPower("MCIPower", 0x000, 8, 0x00),
	  MCIClock("MCIClock", 0x004, 12, 0x00),
	  MCIArgument("MCIArgument", 0x008, 32, 0x00),
	  MMCCommand("MMCCommand", 0x00c, 11, 0x00),
	  MCIRespCmd("MCIRespCmd", 0x010, 6, 0x00),
	  MCIResponse0("MCIResponse0", 0x014, 32, 0x00),
	  MCIResponse1("MCIResponse1", 0x018, 32, 0x00),
	  MCIResponse2("MCIResponse2", 0x01c, 32, 0x00),
	  MCIResponse3("MCIResponse3", 0x020, 31, 0x00),
	  MCIDataTimer("MCIDataTimer", 0x024, 32, 0x00),
	  MCIDataLength("MCIDataLength", 0x028, 16, 0x00),
	  MCIDataCtrl("MCIDataCtrl", 0x02c, 8, 0x00),
	  MCIDataCnt("MCIDataCnt", 0x030, 16, 0x00),
	  MCIStatus("MCIStatus", 0x034, 22, 0x00, true, false),
	  MCIClear("MCIClear", 0x038, 11, 0x00),
	  MCIMask0("MCIMask0", 0x03c, 22, 0x00),
	  MCIMask1("MCIMask1", 0x040, 22, 0x00),
	  MCISelect("MCISelect", 0x044, 4, 0x00),
	  MCIFifoCnt("MCIFifoCnt", 0x048, 15, 0x00),
	  MCIFIFO("MCIFIFO", 0x080, 32, 0x00)
{
	AddRegister(MCIPower);
	AddRegister(MCIClock);
	AddRegister(MCIArgument);
	AddRegister(MMCCommand);
	AddRegister(MCIRespCmd);
	AddRegister(MCIResponse0);
	AddRegister(MCIResponse1);
	AddRegister(MCIResponse2);
	AddRegister(MCIResponse3);
	AddRegister(MCIDataTimer);
	AddRegister(MCIDataLength);
	AddRegister(MCIDataCtrl);
	AddRegister(MCIDataCnt);
	AddRegister(MCIStatus);
	AddRegister(MCIClear);
	AddRegister(MCIMask0);
	AddRegister(MCIMask1);
	AddRegister(MCISelect);
	AddRegister(MCIFifoCnt);
	AddRegister(MCIFIFO);
}

PL180::~PL180()
{

}

bool PL180::Initialise()
{
	return true;
}

uint32_t PL180::ReadRegister(MemoryRegister& reg)
{
	LC_DEBUG1(LogPL180) << "Read Register: " << reg.GetName() << " = " << reg.Read();
	return PrimecellRegisterDevice::ReadRegister(reg);
}

void PL180::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	LC_DEBUG1(LogPL180) << "Write Register: " << reg.GetName() << " = " << std::hex << value << " (old = " << reg.Read() << ")";

	if (reg == MCIClear) {
		MCIStatus.Write(MCIStatus.Read() & ~value);
	} else if (reg == MCIPower) {
		MCIStatus.Write(0xfff);
		PrimecellRegisterDevice::WriteRegister(reg, value);
	} else {
		PrimecellRegisterDevice::WriteRegister(reg, value);
	}

	UpdateIRQs();
}

void PL180::UpdateIRQs()
{
	bool should_assert_0 = (MCIStatus.Read() & MCIMask0.Read()) != 0;
	bool should_assert_1 = (MCIStatus.Read() & MCIMask1.Read()) != 0;

	if (should_assert_0) {
		if (!Getmci0a_pri()->IsAsserted())
			Getmci0a_pri()->Assert();
		if (!Getmci0a_sec()->IsAsserted())
			Getmci0a_sec()->Assert();
	} else {
		if (Getmci0a_pri()->IsAsserted())
			Getmci0a_pri()->Rescind();
		if (Getmci0a_sec()->IsAsserted())
			Getmci0a_sec()->Rescind();
	}

	if (should_assert_1 && !Getmci0b_sec()->IsAsserted())
		Getmci0b_sec()->Assert();
	else if (!should_assert_1 && Getmci0b_sec()->IsAsserted())
		Getmci0b_sec()->Rescind();
}
