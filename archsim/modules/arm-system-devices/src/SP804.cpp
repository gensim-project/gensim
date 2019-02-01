/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "SP804.h"
#include "abi/devices/generic/timing/TickSource.h"
#include "abi/EmulationModel.h"
#include "abi/devices/IRQController.h"
#include "util/LogContext.h"
#include "system.h"
#include <stdio.h>
UseLogContext(LogArmExternalDevice);
UseLogContext(LogTimers);
DeclareChildLogContext(LogSP804, LogArmExternalDevice, "SP804");

using namespace archsim::abi::devices;
using namespace archsim::abi::external::arm;

static ComponentDescriptor sp804_descriptor ("SP804", {{"IRQLine", ComponentParameter_Component}});
SP804::SP804(EmulationModel &parent, Address base_address)
	: PrimecellRegisterDevice(parent, base_address, 0x1000, 0x00141804, 0xb105f00d, "sp804"), Component(sp804_descriptor),
	  ticks(1000), calibrated_ticks(0), recalibrate(0), total_overshoot(0), overshoot_samples(0), suspended(false), emu_model(parent),prescale(0)
{

	timers[0].SetManager(parent.timer_mgr);
	timers[0].id = 0;
	timers[0].SetOwner(*this);

	timers[1].SetManager(parent.timer_mgr);
	timers[1].id = 1;
	timers[1].SetOwner(*this);

	parent.GetSystem().GetTickSource()->AddConsumer(*this);

}

SP804::~SP804()
{
	emu_model.GetSystem().GetTickSource()->RemoveConsumer(*this);
}

bool SP804::Initialise()
{
	return true;
}

bool SP804::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	LC_DEBUG1(LogSP804) << "["<< std::hex  << base_address << "] Read "<< offset << " = ...";

//	fprintf(stderr, "SP804 Read: offset: %x", offset);
	if (offset < 0x20) {
		assert(size == 4);
		return timers[0].ReadRegister(offset, data);
	} else if (offset < 0x40) {
		assert(size == 4);
		return timers[1].ReadRegister(offset - 0x20, data);
	} else {
		return PrimecellRegisterDevice::Read(offset, size, data);
	}
}

bool SP804::Write(uint32_t offset, uint8_t size, uint64_t data)
{
//	fprintf(stderr, "SP804 Write offset: %x, size: %x, data: %x\n", offset, size, data);
	LC_DEBUG1(LogSP804) << "["<< std::hex  << base_address << "] Write "<< offset << " = " << data;
	if (offset < 0x20) {
		assert(size == 4);
		return timers[0].WriteRegister(offset, data);
	} else if (offset < 0x40) {
		assert(size == 4);
		return timers[1].WriteRegister(offset - 0x20, data);
	} else {
		return PrimecellRegisterDevice::Write(offset, size, data);
	}
}

void SP804::UpdateIRQ()
{
	if ((timers[0].GetISR() && timers[0].IsIRQEnabled()) || (timers[1].GetISR() && timers[1].IsIRQEnabled())) {
		if (!GetIRQLine()->IsAsserted() && !suspended) {
			GetIRQLine()->Assert();
		}
	} else {
		if (GetIRQLine()->IsAsserted()) {
			GetIRQLine()->Rescind();
		}
	}
}

void SP804::Tick(uint32_t tick_periods)
{
	tick();
}

void SP804::tick()
{
	if (timers[0].IsEnabled()) timers[0].Tick(ticks);
	if (timers[1].IsEnabled()) timers[1].Tick(ticks);
}


SP804::InternalTimer::InternalTimer() :
	load_value(0),
	current_value(0xffffffff),
	isr(0),
	enabled(false),
	internal_prescale(0),
	ticker(0)
{
	control_reg.value = 0x20;
}

bool SP804::InternalTimer::ReadRegister(uint32_t offset, uint64_t& data)
{
//	fprintf(stderr, "SP804 Internal Timer Read Register\n");
	switch (offset) {
		case 0x0:						// LOAD
			data = load_value;
			return true;
		case 0x4:						// VALUE
			data = current_value;
			return true;

		case 0x08:						// CONTROL
			data = control_reg.value;
			return true;
		case 0x10:						// RIS
			data = isr;
			return true;
		case 0x14:						// MIS
			data = isr & control_reg.bits.int_en;
			return true;
		case 0x18:						// BGLOAD
			data = load_value;
			return true;
	}

	return false;
}

bool SP804::InternalTimer::WriteRegister(uint32_t offset, uint32_t data)
{
//	fprintf(stderr, "SP804 Internal Timer Write Register\n");
	LC_DEBUG1(LogSP804) << "[" << std::hex << owner->base_address << ":" << (uint32_t)id << "] Write " << offset << " = " << data;
	switch (offset) {
		case 0x0:						// LOAD
			load_value = data;
			current_value = data;
			internal_prescale = 0;
			return true;
		case 0x08:						// CONTROL
			control_reg.value = data;
			Update();
			return true;
		case 0x0c:						// INTCLR
			isr = 0;
			owner->UpdateIRQ();
			return true;
		case 0x18:						// BGLOAD
			load_value = data;
			return true;
	}

	return false;
}

/*static void TimerElapsed(archsim::util::timing::TimerCtx timerctx, void *state)
{
	auto timer = (SP804::InternalTimer *)state;
	timer->Tick();
}*/

void SP804::InternalTimer::Update()
{
//	fprintf(stderr, "*** TIMER %d SETTINGS: %x enable=%d, int_en=%d, mode=%d, one_shot=%d, prescale=%d, rsvd=%d, size=%d\n",
//			id, control_reg.value,
//			control_reg.bits.enable,
//			control_reg.bits.int_en,
//			control_reg.bits.mode,
//			control_reg.bits.one_shot,
//			control_reg.bits.prescale,
//			control_reg.bits.rsvd,
//			control_reg.bits.size
//			);

	if (control_reg.bits.enable && !enabled) {
//		assert(!control_reg.bits.one_shot);

		InitialisePeriod();
		internal_prescale = 0;

		LC_DEBUG1(LogSP804) << "Timer Enabled with current value=" << current_value;

		enabled = true;
	} else if (!control_reg.bits.enable && enabled) {
		enabled = false;
		LC_DEBUG1(LogSP804) << "Timer Disabled";
	}

	owner->UpdateIRQ();
}

void SP804::InternalTimer::TickFile()
{
	ticker++;
	if(ticker == (next_irq)) {
		assert(this->IsEnabled());
		isr |= 1;
		if (control_reg.bits.int_en)
			owner->UpdateIRQ();
		InitialisePeriod();
//			fprintf(stderr, "PING IRQ\n");
	}

}

void SP804::InternalTimer::Tick(uint32_t ticks)
{
//	fprintf(stderr, "SP804 Internal Timer Tick\n");
	if (!enabled)
		return;

	/*
	//	if(!internal_qemu_hack) {
	//		internal_qemu_hack = true;
	//		return;
	//	}

		uint32_t prescale_val = 10 * 100;

		internal_prescale += 1;
		if(internal_prescale < prescale_val) {
			return;
		}

		internal_prescale = 0;
	*/
	// Check to see if we've reached zero.
	if (current_value <= ticks) {
		isr |= 1;

		// If the interrupt is enabled, post it.
		if (control_reg.bits.int_en)
			owner->UpdateIRQ();

		// Recalibrate the timer.
		InitialisePeriod();

		// If this is a one-shot timer, then don't restart it.
		if (control_reg.bits.one_shot) {
			return;
		}
	} else {
		current_value -= ticks;
	}
}

void SP804::InternalTimer::InitialisePeriod()
{
//	fprintf(stderr, "SP804 Initialise Period\n");
	if (control_reg.bits.mode == 0) {
		if (control_reg.bits.size == 0) {
			current_value = 0xffff;
		} else {
			current_value = 0xffffffff;
		}
	} else {
		current_value = load_value;
		internal_prescale = 0;
	}
}
