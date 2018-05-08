/*
 * VersatileSIC.cpp
 *
 *  Created on: 16 Dec 2013
 *      Author: harry
 */

#include "VersatileSIC.h"
#include "util/LogContext.h"

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogVersatileSIC, LogArmExternalDevice, "Versatile-SIC");

using namespace archsim::abi::devices;

static ComponentDescriptor versatilesic_descriptor ("VersatileSIC", {{"IRQLine", ComponentParameter_Component}});
template<int nr_lines>
VersatileSIC<nr_lines>::VersatileSIC(EmulationModel& parent, Address base_address)
	: RegisterBackedMemoryComponent(parent, base_address, 0x1000, "sic"),
	  Component(versatilesic_descriptor),
	  IRQController(nr_lines),
	  SIC_STATUS("SIC_STATUS", 0x00, 31, 0),
	  SIC_RAWSTAT("SIC_RAWSTAT", 0x04, 31, 0),
	  SIC_ENABLE("SIC_ENABLE", 0x08, 31, 0),
	  SIC_ENCLR("SIC_ENCLR", 0x0c, 31, 0),
	  SIC_SOFTINTSET("SIC_SOFTINTSET", 0x10, 31, 0),
	  SIC_SOFTINTCLR("SIC_SOFTINTCLR", 0x14, 31, 0),
	  SIC_PICENABLE("SIC_PICENABLE", 0x20, 31, 0),
	  SIC_PICENCLR("SIC_PICENCLR", 0x24, 31, 0)
{
	AddRegister(SIC_STATUS);
	AddRegister(SIC_RAWSTAT);
	AddRegister(SIC_ENABLE);
	AddRegister(SIC_ENCLR);
	AddRegister(SIC_SOFTINTSET);
	AddRegister(SIC_SOFTINTCLR);
	AddRegister(SIC_PICENABLE);
	AddRegister(SIC_PICENCLR);

	for (int i = 0; i < nr_lines; i++) {
		lines[i] = false;
	}
}

template<int nr_lines>
VersatileSIC<nr_lines>::~VersatileSIC()
{

}

template <int nr_lines>
bool VersatileSIC<nr_lines>::Initialise()
{
	return true;
}

template<int nr_lines>
uint32_t VersatileSIC<nr_lines>::ReadRegister(MemoryRegister& reg)
{
	LC_DEBUG1(LogVersatileSIC) << "Read Register: " << reg.GetName();

	if (reg == SIC_STATUS) {
		uint32_t lines_active = 0;
		for (int i = 0; i < nr_lines; i++) {
			if (lines[i]) lines_active |= (1 << i);
		}

		return lines_active & SIC_ENABLE.Read();
	} else if (reg == SIC_RAWSTAT) {
		uint32_t lines_active = 0;
		for (int i = 0; i < nr_lines; i++) {
			if (lines[i]) lines_active |= (1 << i);
		}

		return lines_active;
	} else {
		return RegisterBackedMemoryComponent::ReadRegister(reg);
	}
}

template<int nr_lines>
void VersatileSIC<nr_lines>::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	LC_DEBUG1(LogVersatileSIC) << "Write Register: " << reg.GetName();
	if (reg == SIC_ENABLE) {
		SIC_ENABLE.Write(SIC_ENABLE.Read() | value);
	} else if (reg == SIC_ENCLR) {
		SIC_ENABLE.Write(SIC_ENABLE.Read() & ~value);
	} else {
		RegisterBackedMemoryComponent::WriteRegister(reg, value);
	}
}


template<int nr_lines>
bool VersatileSIC<nr_lines>::AssertLine(uint32_t line)
{
	assert(line < nr_lines);
	LC_DEBUG1(LogVersatileSIC) << "Asserting Line " << line;

	lines[line] = true;
	UpdateParentState();

	return true;
}

template<int nr_lines>
bool VersatileSIC<nr_lines>::RescindLine(uint32_t line)
{
	assert(line < nr_lines);
	LC_DEBUG1(LogVersatileSIC) << "Rescinding Line " << line;

	lines[line] = false;
	UpdateParentState();

	return true;
}

template<int nr_lines>
void VersatileSIC<nr_lines>::UpdateParentState()
{
	bool have_asserted_lines = false;

	for (int i = 0; i < nr_lines; i++) {
		have_asserted_lines |= lines[i] && (SIC_ENABLE.Read() & (1 << i));
	}

	IRQLine *irq = GetIRQLine();

	if (have_asserted_lines && !irq->IsAsserted()) {
		irq->Assert();
	} else if (!have_asserted_lines && irq->IsAsserted()) {
		irq->Rescind();
	}
}

template class VersatileSIC<32>;
