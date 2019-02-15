/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/riscv/SifiveUART.h"

using namespace archsim::abi::devices::riscv;

DeclareLogContext(LogSifiveUART, "SifiveUART");

using namespace archsim::abi::devices;
static ComponentDescriptor SifiveUARTDescriptor ("SifiveUART", {{"IRQLine", ComponentParameter_Component}, {"SerialPort", ComponentParameter_Component}});

static void console_callback(AsyncSerial *serial, void *ctx)
{
	SifiveUART *pl011 = (SifiveUART*)ctx;
	char data;
	serial->ReadChar(data);
	pl011->EnqueueChar(data);
}


SifiveUART::SifiveUART(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x1000), Component(SifiveUARTDescriptor), serial_(nullptr), ie_(0)
{

}

SifiveUART::~SifiveUART()
{

}

bool SifiveUART::Initialise()
{
	ie_ = 0;
	rxctrl_ = 0;
	txctrl_ = 0;
	ip_ = 0;
	div_ = 0;
	return true;
}

bool SifiveUART::EnqueueChar(char c)
{
	rx_fifo_.push(c);
	UpdateIRQ();

	return true;
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
			data = ReadRXFifo();
			UpdateIRQ();
			break;
		case 0x08: // txctrl
			data = txctrl_;
			break;
		case 0x0c: // rxctrl
			data = rxctrl_;
			break;
		case 0x10: // ie
			data = ie_;
			break;
		case 0x14: // ip
			data = ip_;
			break;
		case 0x18: // div
			data = div_;
			break;
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
			LC_DEBUG1(LogSifiveUART) << "TXCTRL set to " << Address(data);
			txctrl_ = data;
			break;
		case 0x0c: // rxctrl
			LC_DEBUG1(LogSifiveUART) << "RXCTRL set to " << Address(data);
			rxctrl_ = data;
			break;
		case 0x10: // ie
			LC_DEBUG1(LogSifiveUART) << "IE set to " << Address(data);
			ie_ = data;
			break;
		case 0x14: // ip
			UNIMPLEMENTED;
		case 0x18: // div
			div_ = data;
			break;
		default:
			UNEXPECTED;
	}

	UpdateIRQ();
	return true;
}

void SifiveUART::UpdateIRQ()
{
	uint32_t old_ip = ip_;
	ip_ = 0;

	// if txwm interrupt is enabled, then pend it
	uint32_t txcnt = (txctrl_ >> 16) & 3;
	if(txcnt > 0) {
		// pend txwm interrupt
		ip_ |= (1 << 0);
	}

	// rxwm: todo
	uint32_t rxcnt = (rxctrl_ >> 16) & 3;
	if(rxcnt < rx_fifo_.size()) {
		// pend txwm interrupt
		ip_ |= (1 << 1);
	}

	if(ip_ != old_ip) {
		LC_DEBUG1(LogSifiveUART) << "IP set to " << Address(ip_);
	}

	if(ip_ & ie_) {
		if(!GetIRQLine()->IsAsserted()) {
			LC_DEBUG1(LogSifiveUART) << "Pending IRQ!";
			GetIRQLine()->Assert();
		}
	} else {
		if(GetIRQLine()->IsAsserted()) {
			LC_DEBUG1(LogSifiveUART) << "Rescinding IRQ.";
			GetIRQLine()->Rescind();
		}
	}
}

uint32_t SifiveUART::ReadRXFifo()
{
	uint32_t data = 0;
	if(!rx_fifo_.empty()) {
		data = rx_fifo_.front();
		rx_fifo_.pop();
	} else {
		data = 0x80000000;
	}

	LC_DEBUG1(LogSifiveUART) << "Read " << Address(data) << " out of read buffer";

	UpdateIRQ();

	return data;
}
