
#include "abi/devices/Device.h"
#include "abi/devices/Component.h"

#include "util/LogContext.h"

#include <cassert>
#include <cstdint>

DeclareLogContext(LogDevice, "Device");

DefineComponentType(archsim::abi::devices::Device);

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			Device::Device() : Manager(NULL)
			{

			}

			Device::~Device()
			{
			}

			bool Device::Configure(gensim::Processor *core, std::map<std::string,std::string>& settings)
			{
				return true;
			}

			bool Device::Read8(uint32_t address, uint8_t& value)
			{
				return false;
			}

			bool Device::Read16(uint32_t address, uint16_t& value)
			{
				return false;
			}

			bool Device::Read32(uint32_t address, uint32_t& value)
			{
				return false;
			}

			bool Device::Read64(uint32_t address, uint64_t& value)
			{
				return false;
			}

			bool Device::ReadBlock(uint32_t address, void *data, unsigned int length)
			{
				return false;
			}

			bool Device::Write8(uint32_t address, uint8_t value)
			{
				return false;
			}

			bool Device::Write16(uint32_t address, uint16_t value)
			{
				return false;
			}

			bool Device::Write32(uint32_t address, uint32_t value)
			{
				return false;
			}

			bool Device::Write64(uint32_t address, uint64_t value)
			{
				return false;
			}

			bool Device::WriteBlock(uint32_t address, const void *data, unsigned int length)
			{
				return false;
			}

			bool Device::SetManager(PeripheralManager* new_manager)
			{
				if (Manager) return false;
				Manager = new_manager;
				return true;
			}

		}
	}
}
