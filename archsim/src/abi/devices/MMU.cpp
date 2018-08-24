/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * MMU.cpp
 *
 *	Created on: 17 Jun 2014
 *			Author: harry
 */


#include "system.h"
#include "abi/devices/MMU.h"
#include "abi/EmulationModel.h"
#include "util/LogContext.h"

UseLogContext(LogDevice)

using namespace archsim::abi::devices;
using namespace archsim::abi::memory;
using archsim::Address;

PageInfo::PageInfo() : phys_addr(0), mask(0), Present(0), UserCanRead(0), UserCanWrite(0), KernelCanRead(0), KernelCanWrite(0) {}

MMU::MMU() : should_be_enabled(false) { }

MMU::~MMU() {}

MMU::TranslateResult MMU::TranslateRegion(archsim::core::thread::ThreadInstance *cpu, Address virt_addr, uint32_t size, Address &phys_addr, const struct AccessInfo info)
{
	return TXLN_FAULT_OTHER;
}

void MMU::FlushCaches()
{

}

void MMU::Evict(Address virt_addr)
{

}

void MMU::set_enabled(bool enabled)
{
	if (enabled != should_be_enabled) {
		LC_DEBUG1(LogDevice) << "MMU Enablement set to " << enabled;

		should_be_enabled = enabled;

		Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
		Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);
	}
}

Address MMU::TranslateUnsafe(archsim::core::thread::ThreadInstance* cpu, Address virt_addr)
{
	Address phys_addr;
	AccessInfo info(0, 0, 0);

	Translate(cpu, virt_addr, phys_addr, info);
	return phys_addr;
}

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			std::ostream &operator <<(std::ostream &stream, const struct archsim::abi::devices::AccessInfo &info)
			{
				stream << "[";
				if(info.Fetch) stream << "I";
				else stream << "D";
				if(info.Kernel) stream << "K";
				else stream << "U";
				if(info.Write) stream << "W";
				else stream << "R";

				stream << "]";
				return stream;
			}
		}
	}
}
