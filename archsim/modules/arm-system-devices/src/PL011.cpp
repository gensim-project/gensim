/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PL011.cpp
 *
 *  Created on: 9 Jan 2014
 *      Author: harry
 */

/* PL011 UART */

#include "PL011.h"
#include "abi/devices/IRQController.h"
#include "util/LogContext.h"

#include <cassert>
#include <iomanip>
#include <stdlib.h>

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL011, LogArmExternalDevice, "PL011");
DeclareChildLogContext(LogPL011IRQs, LogPL011, "IRQ");

#define IRQ_TXINTR (1 << 5)
#define IRQ_RXINTR (1 << 4)

using namespace archsim::abi::external::arm;
using namespace archsim::abi::devices;

static void console_callback(AsyncSerial *serial, void *ctx)
{
	PL011 *pl011 = (PL011*)ctx;
	char data;
	serial->ReadChar(data);
	pl011->EnqueueChar(data);
	LC_DEBUG1(LogPL011) << "Async message received";
}

static ComponentDescriptor pl011_descriptor ("PL011", {{"IRQLine", ComponentParameter_Component}, {"SerialPort", ComponentParameter_Component}});
PL011::PL011(EmulationModel &parent, Address base_addr) :  MemoryComponent(parent, base_addr, 0x1000), Component(pl011_descriptor), control_word(0x0300), baud_rate(0), fractional_baud(0), irq_mask(0), irq_status(0), serial(nullptr)
{
}

PL011::~PL011()
{
	if(GetSerialPort())GetSerialPort()->Close();
}

bool PL011::Initialise()
{
	return true;
}

AsyncSerial *PL011::GetSerial()
{
	if(serial == nullptr && GetSerialPort() != nullptr) {
		serial = new AsyncSerial(GetSerialPort(), console_callback, this);
		serial->Open();
	}
	return serial;
}

void PL011::EnqueueChar(char c)
{
	fifo.push_back(c);
	RaiseRxIRQ();
}

bool PL011::Read(uint32_t offset, uint8_t size, uint32_t& data)
{
	uint32_t reg = (offset & 0xfff);
	switch(reg) {
		case 0x000: //Data register
			if(fifo.empty()) data = 0;
			else {
				data = fifo.front();
				fifo.pop_front();
				irq_status &= ~(IRQ_RXINTR);
				CheckIRQs();
			}
			break;
		case 0x004: //rx status register
			data = 0;
			break;
		case 0x018: // Flag register
			data = 0;
			if(fifo.empty()) data |= (1 << 4); //RXFE
			data |= (1 << 7); //TXFE (always)
			LC_DEBUG1(LogPL011) << "Reading flag register: " << std::hex << data;
			break;
		case 0x024: //integer baud rate register
			data = baud_rate;
			break;
		case 0x028: //fractional baud rate register
			data = fractional_baud;
			break;
		case 0x02c: //line control register
			data = line_control;
			break;
		case 0x030: // Control register
			data = control_word;
			break;
		case 0x34: //UARTIFLS
			data = ifl;
			break;
		case 0x038:
			data = irq_mask;
			break;
		case 0x03C:
			LC_DEBUG1(LogPL011) << "Reading raw IRQ Status register" << std::hex << (irq_status);
			data = irq_status;
			break;
		case 0x040:
			LC_DEBUG1(LogPL011) << "Reading masked IRQ Status register: " << std::hex << (irq_status & irq_mask);
			data = irq_status & irq_mask;
			break;
		// ID Registers
		case 0xFE0:
			data = 0x11;
			break;
		case 0xFE4:
			data = 0x10;
			break;
		case 0xFE8:
			data = 0x14;
			break;
		case 0xFEC:
			data = 0x00;
			break;
		case 0xFF0:
			data = 0x0D;
			break;
		case 0xFF4:
			data = 0xF0;
			break;
		case 0xFF8:
			data = 0x05;
			break;
		case 0xFFC:
			data = 0xB1;
			break;
		default:
			LC_WARNING(LogPL011) << "Attempted to read unimplemented/non-existant register " << std::hex << reg;
			return false;
	}
	return true;
}

bool PL011::Write(uint32_t offset, uint8_t size, uint32_t data)
{
	uint32_t reg = (offset & 0xfff);
	switch(reg) {
		case 0x000: // Data register
			if(GetSerial())GetSerial()->WriteChar(data & 0xff);
			RaiseTxIRQ();
			break;
		case 0x004: // RX status register / clear error register
			control_word = data;
			break;
		case 0x018: // Flag register
			break;
		case 0x024: // Integer baud rate register
			baud_rate = data & 0xffff;
			break;
		case 0x028:
			fractional_baud = data & 0x3f;
			break;
		case 0x02C:
			line_control = data & 0xff;
			break;
		case 0x030: // Control register
			control_word = data;
			break;
		case 0x34:
			ifl = data;
			break;
		case 0x38:
			irq_mask = data & 0x7ff;
			CheckIRQs();
			break;
		case 0x044:
			LC_DEBUG1(LogPL011IRQs) << "Clearing IRQ: Prev status " << std::hex << irq_status << ", clearing " << data;
			irq_status = irq_status & ~data;
			CheckIRQs();
			break;
		case 0x048:
			LC_ERROR(LogPL011) << "Unimplemented DMA request";
			assert(false);
			break;
		default:
			LC_WARNING(LogPL011) << "Attempted to write unimplemented/non-existant register " << std::hex << reg << " = " << data << "(" << (uint32_t)size << ")";
			break;
	}
	return false;
}

void PL011::RaiseTxIRQ()
{
	irq_status |= IRQ_TXINTR;

	CheckIRQs();
}

void PL011::RaiseRxIRQ()
{
	LC_DEBUG1(LogPL011IRQs) << "Raising RX IRQ";
	irq_status |= IRQ_RXINTR;
	CheckIRQs();
}

void PL011::CheckIRQs()
{
	if(irq_status & irq_mask) {
		if(!GetIRQLine()->IsAsserted()) {
			LC_DEBUG1(LogPL011IRQs) << "Asserting IRQ. IRQ status: " << std::hex << std::setw(2) << std::setfill('0') << irq_status << ", Mask:" << irq_mask;
			GetIRQLine()->Assert();
		}
	} else {
		if(GetIRQLine()->IsAsserted()) {
			LC_DEBUG1(LogPL011IRQs) << "Rescinding IRQ. IRQ status: " << std::hex << std::setw(2) << std::setfill('0') << irq_status << ", Mask:" << irq_mask;
			GetIRQLine()->Rescind();
		}
	}
}
