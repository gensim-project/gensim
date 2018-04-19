
#include "gensim/gensim_processor.h"

#include "abi/devices/PeripheralManager.h"
#include "abi/devices/Device.h"
#include "system.h"

#include "util/LogContext.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			ComponentDescriptor peripheral_manager_descriptor("PeripheralManager");

			PeripheralManager::PeripheralManager(archsim::ThreadInstance &_cpu) : Component(peripheral_manager_descriptor), cpu(_cpu), emu_model(NULL)
			{
				assert(&cpu);
			}

			bool PeripheralManager::CoreProbeDevice(uint32_t device_id)
			{
				return AttachedPeripherals.find(device_id) != AttachedPeripherals.end();
			}

			bool PeripheralManager::CoreReadDevice(uint32_t device_id, uint32_t address, uint32_t &data)
			{
				if(AttachedPeripherals.count(device_id) == 0) return false;
				return AttachedPeripherals.at(device_id)->Read32(address, data);
			}

			bool PeripheralManager::CoreWriteDevice(uint32_t device_id, uint32_t address, uint32_t data)
			{
				if(AttachedPeripherals.count(device_id) == 0) return false;
				return AttachedPeripherals.at(device_id)->Write32(address, data);
			}

			bool PeripheralManager::ProbeDeviceByName(std::string name)
			{
				return Peripherals.find(name) != Peripherals.end();
			}

			Device* PeripheralManager::GetDeviceByName(std::string name)
			{
				if(Peripherals.find(name) != Peripherals.end()) return Peripherals.at(name);
				return NULL;
			}

			bool PeripheralManager::InitialiseDevices()
			{
				emu_model = &cpu.GetEmulationModel();
				bool success = true;
				for(auto device : Peripherals) {
					success &= device.second->Initialise();
				}

				return success;
			}

			bool PeripheralManager::RegisterDevice(std::string name, Device* device)
			{
				if(ProbeDeviceByName(name)) return false;
				device->SetManager(this);
				Peripherals[name] = device;
				return true;
			}

			bool PeripheralManager::AttachDevice(std::string name, uint32_t id)
			{
				Device *device = GetDeviceByName(name);
				if(!device) return false;
				AttachedPeripherals[id] = device;
				return true;
			}

			bool PeripheralManager::DetachDevice(uint32_t id)
			{
				if(!CoreProbeDevice(id)) return false;
				AttachedPeripherals.erase(id);
				return true;
			}



		}
	}
}
