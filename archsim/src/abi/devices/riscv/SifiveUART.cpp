/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/riscv/SifiveUART.h"

using namespace archsim::abi::devices::riscv;

using namespace archsim::abi::devices;
static ComponentDescriptor SifiveUARTDescriptor ("SifiveUART", {{"IRQLine", ComponentParameter_Component}, {"SerialPort", ComponentParameter_Component}});

static void console_callback(AsyncSerial *serial, void *ctx)
{
	SifiveUART *pl011 = (SifiveUART*)ctx;
	char data;
	serial->ReadChar(data);
	pl011->EnqueueChar(data);
}


SifiveUART::SifiveUART(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x1000), Component(SifiveUARTDescriptor), serial_(nullptr)
{

}

SifiveUART::~SifiveUART()
{

}

bool SifiveUART::Initialise()
{
	return true;
}

bool SifiveUART::EnqueueChar(char c)
{
	UNIMPLEMENTED;
}

AsyncSerial *SifiveUART::GetSerial()
{
	if(serial_ == nullptr && GetSerialPort() != nullptr) {
		serial_ = new AsyncSerial(GetSerialPort(), console_callback, this);
		serial_->Open();
	}
	return serial_;
}


bool SifiveUART::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	offset &= 0xff;

	switch(offset) {
		case 0x00: // txdata
			data = 0;
			break;
		case 0x04: // rxdata
			data = 0x80000000;
			break;
		case 0x08: // txctrl
			data = txctrl_;
			break;
		case 0x0c: // rxctrl
			data = rxctrl_;
			break;
		case 0x10: // ie
		case 0x14: // ip
		case 0x18: // div
			UNIMPLEMENTED;
		default:
			UNEXPECTED;
	}

	return true;
}

bool SifiveUART::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	offset &= 0xff;

	switch(offset) {
		case 0x00: // txdata
			GetSerial()->WriteChar(data);
			break;
		case 0x04: // rxdata
			break;
		case 0x08: // txctrl
			txctrl_ = data;
			break;
		case 0x0c: // rxctrl
			rxctrl_ = data;
			break;
		case 0x10: // ie
		case 0x14: // ip
		case 0x18: // div
			UNIMPLEMENTED;
		default:
			UNEXPECTED;
	}

	return true;
}
