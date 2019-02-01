/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
/*
 * PL110.cpp
 *
 *  Created on: 21 Jul 2014
 *      Author: harry
 */

#include "PL110.h"
#include "util/LogContext.h"

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL110, LogArmExternalDevice, "PL110");

//ARM PL110 LCDC LCD Controller
//Note that this implements the modified PL110 for the ARM Versatile AB, not the standard PL110.
//Register differences are outlined in DUI0225D
using namespace archsim::abi::devices;

static ComponentDescriptor pl110_descriptor("pl110", {{"Screen", ComponentParameter_Component}});
PL110::PL110(EmulationModel &parent, Address base_addr) : MemoryComponent(parent, base_addr, 0x10000), Component(pl110_descriptor), irq(NULL), timing {0,0,0,0}, upper_fbbase(0), lower_fbbase(0), control(0), irq_mask(0), irq_raw(0), internal_enabled(false)
{
	LC_DEBUG2(LogPL110) << "Created PL110";
}

PL110::~PL110() {}

bool PL110::Initialise()
{
	return true;
}

bool PL110::Read(uint32_t offset, uint8_t size, uint64_t &data)
{
	offset &= 0xfff;
	LC_DEBUG2(LogPL110) << "Reading from register " << std::hex << offset;

	if(offset >= 0x200 && offset < 0x400) {
		data = raw_palette[(offset - 0x200) >> 2];
		return true;
	}

	switch(offset) {
		case 0x000: //LCDTiming0
			data = timing[0];
			break;
		case 0x004: //LCDTiming1
			data = timing[1];
			break;
		case 0x008: //LCDTiming2
			data = timing[2];
			break;
		case 0x00C: //LCDTiming3
			data = timing[3];
			break;
		case 0x010: //LCDUPBASE
			data = upper_fbbase;
			break;
		case 0x014: //LCDLPBASE
			data = lower_fbbase;
			break;
		case 0x018: //LCDControl (Register switched in modified PL110)
			data = control;
			break;
		case 0x01C: //LCDIMSC (Register switched in modified PL110)
			data = irq_mask;
			break;
		case 0x020: //LCDRIS
			data = irq_raw;
			break;
		case 0x024: //LCDMIS
			data = irq_raw & irq_mask;
			break;
		case 0x028: //LCDICR
			LC_WARNING(LogPL110) << "Attempted to read a write-only register (LCDICR)";
			break;
		case 0x02C: //LCDUPCURR
			data = upper_fbbase;
			break;
		case 0x030: //LCDLPCURR
			data = lower_fbbase;
			break;

		//TODO: Palette registers


		//Although we implement the Versatile-AB PL110, and the documentation
		//states that it has a different peripheral ID to the standard PL110,
		//this is actually a lie and in actuality it has the same ID as a
		//standard PL110.
		case 0xFE0: //CLCDPERIPHID0
			data = 0x11;
			break;
		case 0xFE4: //CLCDPERIPHID1
			data = 0x11;
			break;
		case 0xFE8: //CLCDPERIPHID2
			data = 0x24;
			break;
		case 0xFEC: //CLCDPERIPHID3
			data = 0x00;
			break;
		case 0xFF0: //CLCDPCELLID0
			data = 0x0D;
			break;
		case 0xFF4: //CLCDPCELLID1
			data = 0xF0;
			break;
		case 0xFF8: //CLCDPCELLID2
			data = 0x05;
			break;
		case 0xFFC: //CLCDPCELLID3
			data = 0xB1;
			break;
		default:
			LC_ERROR(LogPL110) << "Attempted to read from unknown register " << std::hex << offset;
	}
	return true;
}

bool PL110::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	offset &= 0xfff;
	LC_DEBUG2(LogPL110) << "Writing to register " << std::hex << offset;

	if(offset >= 0x200 && offset < 0x400) {
		raw_palette[(offset - 0x200) >> 2] = data;
		update_palette();
		return true;
	}

	switch(offset) {
		case 0x000: //LCDTiming0
			timing[0] = data;
			internal_ppl = 16 * (1+((data >> 2) & 0x3f));
			if(internal_enabled) {
				disable_screen();
				enable_screen();
			}
			LC_DEBUG1(LogPL110) << "Pixels Per Line set to " << internal_ppl;
			break;
		case 0x004: //LCDTiming1
			timing[1] = data;
			internal_lines = (data & 0x3ff) + 1;
			if(internal_enabled) {
				disable_screen();
				enable_screen();
			}
			LC_DEBUG1(LogPL110) << "Lines per panel set to " << internal_lines;
			break;
		case 0x008: //LCDTiming2
			timing[2] = data;
			break;
		case 0x00C: //LCDTiming3
			timing[3] = data;
			break;
		case 0x010: //LCDUPBASE
			upper_fbbase = data;
			if(GetScreen()) GetScreen()->SetFramebufferPointer(Address(upper_fbbase));
			break;
		case 0x014: //LCDLPBASE
			lower_fbbase = data;
			break;
		case 0x018: //LCDControl (Register switched in modified PL110)
			LC_DEBUG1(LogPL110) << "Updated control register to " << std::hex << data;
			control = data;
			update_control();
			break;
		case 0x01C: //LCDIMSC (Register switched in modified PL110)
			irq_mask = data;
			update_irq();
			break;
		case 0x020: //LCDRIS
			LC_WARNING(LogPL110) << "Attempted to write a read-only register (LCDRIS)";
			break;
		case 0x024: //LCDMIS
			LC_WARNING(LogPL110) << "Attempted to write a read-only register (LCDMIS)";
			break;
		case 0x028: //LCDICR
			irq_raw = irq_raw & ~data;
			update_irq();
			break;
		case 0x02C: //LCDUPCURR
			LC_WARNING(LogPL110) << "Attempted to write a read-only register (LCDUPCURR)";
			break;
		case 0x030: //LCDLPCURR
			LC_WARNING(LogPL110) << "Attempted to write a read-only register (LCDLPCURR)";
			break;
		default:
			LC_ERROR(LogPL110) << "Attempted to write to unimplemented register " << std::hex << offset << " = " << data;
			break;
	}
	return false;
}

void PL110::update_irq()
{
	if(irq == NULL) return;
	if(irq_raw & irq_mask) irq->Assert();
	else irq->Rescind();
}

void PL110::update_control()
{
	if((control & 0x1) && (control & (1 << 11))) enable_screen();
	GetScreen()->SetFramebufferPointer(Address(upper_fbbase));
}

void PL110::update_palette()
{
	GetScreen()->SetPaletteMode(gfx::PaletteDirect);
	GetScreen()->SetPaletteSize(256);

	for(int i = 0; i < (0x200>>2); ++i) {
		uint16_t entry_1 = raw_palette[i] & 0xffff;
		uint16_t entry_2 = raw_palette[i] >> 16;

		GetScreen()->SetPaletteEntry(i*2, entry_1);
		GetScreen()->SetPaletteEntry((i*2)+1, entry_2);
	}
}

void PL110::enable_screen()
{
	if(!GetScreen()) {
		LC_WARNING(LogPL110) << "Attempted to bring up the screen when no screen was connected";
		return;
	}
	if(internal_enabled) return;
	internal_enabled = true;

	gfx::VirtualScreenMode mode;

	uint32_t lcdbpp = (control >> 1) & 0x7;
	switch(lcdbpp) {
		case 0:
			mode = gfx::VSM_None;
			break; //TODO: 1bit
		case 1:
			mode = gfx::VSM_None;
			break; //TODO: 2bit
		case 2:
			mode = gfx::VSM_None;
			break; //TODO: 4bit
		case 3:
			mode = gfx::VSM_8Bit;
			break;
		case 4:
			mode = gfx::VSM_16bit;
			break;
		case 5:
			mode = gfx::VSM_None;
			break; //TODO: 24bit
		case 6:
			mode = gfx::VSM_16bit;
			break;
		default:
			mode = gfx::VSM_None;
			break;
	}

	LC_DEBUG1(LogPL110) << "Initialising screen " << internal_ppl << "x" << internal_lines << " in mode " << mode << "(lcdbpp = " << lcdbpp << ")";

	if(GetScreen()->Configure(internal_ppl, internal_lines, mode)) {
		if(!GetScreen()->Initialise()) {
			LC_ERROR(LogPL110) << "Failed to initialise screen";
		}
	} else LC_ERROR(LogPL110) << "Failed to initialise screen";
}

void PL110::disable_screen()
{
	if(!internal_enabled) return;
	internal_enabled = false;
	if(GetScreen()) GetScreen()->Reset();
}
