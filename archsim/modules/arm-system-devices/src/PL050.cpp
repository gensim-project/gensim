/*
 * abi/devices/arm/external/PL050.cpp
 */
#include "PL050.h"
#include "abi/devices/generic/ps2/PS2Device.h"
#include "abi/devices/IRQController.h"
#include "util/LogContext.h"

#define PL050_TXEMPTY         (1 << 6)
#define PL050_TXBUSY          (1 << 5)
#define PL050_RXFULL          (1 << 4)
#define PL050_RXBUSY          (1 << 3)
#define PL050_RXPARITY        (1 << 2)
#define PL050_KMIC            (1 << 1)
#define PL050_KMID            (1 << 0)

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL050, LogArmExternalDevice, "PL050");

using namespace archsim::abi::devices;
using namespace archsim::abi::external::arm;

static ComponentDescriptor pl050_descriptor ("PL050", {{"PS2", ComponentParameter_Component}});
PL050::PL050(EmulationModel& parent, Address base_address)
	: PrimecellRegisterDevice(parent, base_address, 0x1000, 0x00041050, 0xb105f00d, "pl050"),
	  Component(pl050_descriptor),
	  KMICR("KMICR", 0x00, 6, 0x00),
	  KMISTAT("KMISTAT", 0x04, 7, 0x43, true, false),
	  KMIDATA("KMIDATA", 0x08, 8, 0x00),
	  KMICLKDIV("KMICLKDIV", 0x0c, 4, 0x00),
	  KMIIR("KMIIR", 0x10, 2, 0x00, true, false)
{
	AddRegister(KMICR);
	AddRegister(KMISTAT);
	AddRegister(KMIDATA);
	AddRegister(KMICLKDIV);
	AddRegister(KMIIR);
}

PL050::~PL050()
{

}

bool PL050::Initialise()
{
	return true;
}

void PL050::WriteRegister(MemoryRegister& reg, uint32_t new_value)
{
	if (reg == KMICR) {
		assert(!(new_value & 0x8));

		if ((new_value & 0x10) == 0x10) {
			GetPS2()->EnableIRQ();
		} else {
			GetPS2()->DisableIRQ();
		}

		reg.Write(new_value);
	} else if (reg == KMIDATA) {
		GetPS2()->SendCommand(new_value & 0xff);
	} else {
		PrimecellRegisterDevice::WriteRegister(reg, new_value);
	}
}

uint32_t PL050::ReadRegister(MemoryRegister& reg)
{
	if (reg == KMISTAT) {
		uint8_t val;
		uint32_t stat;

		val = last;
		val = val ^ (val >> 4);
		val = val ^ (val >> 2);
		val = (val ^ (val >> 1)) & 1;

		stat = PL050_TXEMPTY;
		if (val)
			stat |= PL050_RXPARITY;
		if (GetPS2()->DataPending())
			stat |= PL050_RXFULL;

		return stat;
	} else if (reg == KMIDATA) {
		LC_DEBUG1(LogPL050) << std::hex << base_address << "Reading data register: Pending=" << GetPS2()->DataPending() << ", Last=" << last;
		if (GetPS2()->DataPending()) {
			last = GetPS2()->Read();
			LC_DEBUG1(LogPL050) << std::hex << base_address << "Reading data register: New Data=" << last;
		}
		return last;
	} else if (reg == KMIIR) {
		return (GetPS2()->DataPending() ? 1 : 0) | 2;
	} else {
		return PrimecellRegisterDevice::ReadRegister(reg);
	}
}
