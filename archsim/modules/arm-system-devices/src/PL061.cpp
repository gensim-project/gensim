/*
 * abi/devices/arm/external/PL061.cpp
 */
#include "PL061.h"
#include "util/LogContext.h"

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL061, LogArmExternalDevice, "PL061");

using namespace archsim::abi::devices;
using namespace archsim::abi::external::arm;

static ComponentDescriptor pl061_descriptor("PL061", {{"IRQLine", ComponentParameter_Component}});
PL061::PL061(EmulationModel& parent, Address base_address)
	: PrimecellRegisterDevice(parent, base_address, 0x1000, 0x00041061, 0xb105f00d, "pl061"),
	  Component(pl061_descriptor),
	  GPIODir("GPIODIR", 0x400, 8, 0x00),
	  GPIOIS("GPIOIS", 0x404, 8, 0x00),
	  GPIOIBE("GPIOIBE", 0x408, 8, 0x00),
	  GPIOIEV("GPIOIEV", 0x40c, 8, 0x00),
	  GPIOIE("GPIOIE", 0x410, 8, 0x00),
	  GPIORIS("GPIORIS", 0x414, 8, 0x00),
	  GPIOMIS("GPIOMIS", 0x418, 8, 0x00),
	  GPIOIC("GPIOIC", 0x41c, 8, 0x00),
	  GPIOAFSEL("GPIOAFSEL", 0x420, 8, 0x00)
{
	AddRegister(GPIODir);
	AddRegister(GPIOIS);
	AddRegister(GPIOIBE);
	AddRegister(GPIOIEV);
	AddRegister(GPIOIE);
	AddRegister(GPIORIS);
	AddRegister(GPIOMIS);
	AddRegister(GPIOIC);
	AddRegister(GPIOAFSEL);
}

PL061::~PL061()
{

}

bool PL061::Initialise()
{
	return true;
}

uint32_t PL061::ReadRegister(MemoryRegister& reg)
{
	LC_DEBUG1(LogPL061) << "Read Register: " << reg.GetName() << " = " << reg.Read();
	return PrimecellRegisterDevice::ReadRegister(reg);
}

void PL061::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	LC_DEBUG1(LogPL061) << "Write Register: " << reg.GetName() << " = " << std::hex << value << " (old = " << reg.Read() << ")";
	PrimecellRegisterDevice::WriteRegister(reg, value);
}

bool PL061::Read(uint32_t offset, uint8_t size, uint32_t& data)
{
	if (offset >= 0 && offset <= 0x3fc) {
		return HandleDataRead(offset, size, data);
	}

	return RegisterBackedMemoryComponent::Read(offset, size, data);
}

bool PL061::HandleDataRead(uint32_t offset, uint8_t size, uint32_t& data)
{
	LC_DEBUG1(LogPL061) << "Read Data: " << std::hex << offset;
	data = 0;
	return true;
}

generic::GPIO* PL061::AttachGPIO(unsigned int idx)
{
	return NULL;
}
