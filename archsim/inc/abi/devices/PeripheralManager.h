/*
 * File:   PeripheralManager.h
 * Author: harry
 *
 * Created on 11 December 2013, 15:39
 */

#ifndef PERIPHERALMANAGER_H
#define	PERIPHERALMANAGER_H

#include <map>
#include <string>

#include "abi/devices/Component.h"

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace core {
		namespace thread {
			class ThreadInstance;
		}
	}
	namespace abi
	{

		class EmulationModel;

		namespace devices
		{

			class Device;

			class PeripheralManager : public Component
			{
			public:
				PeripheralManager(archsim::core::thread::ThreadInstance &cpu);

				bool Initialise()
				{
					return true;
				}

				archsim::core::thread::ThreadInstance &cpu;

				// Map by name of all peripherals attached to this peripheral manager.
				// There peripherals are not necessarily attached directly to the core
				// but can communicate between each other and the emulation model
				std::map<std::string, Device*> Peripherals;

				// Map by device ID of peripherals which are directly attached to the
				// core. The core is able to probe for, read from and write to these
				// devices
				std::map<uint32_t, Device*> AttachedPeripherals;

				bool CoreProbeDevice(uint32_t device_id);
				bool CoreReadDevice(uint32_t device_id, uint32_t address, uint32_t &data);
				bool CoreWriteDevice(uint32_t device_id, uint32_t address, uint32_t data);

				bool ProbeDeviceByName(std::string name);
				Device* GetDeviceByName(std::string name);
				Device *GetDevice(uint32_t device_id) { return AttachedPeripherals.at(device_id); }

				bool InitialiseDevices();

				bool RegisterDevice(std::string name, Device* device);
				bool AttachDevice(std::string name, uint32_t id);
				bool DetachDevice(uint32_t id);

			private:
				EmulationModel *emu_model;

			public:
				inline archsim::abi::EmulationModel *GetEmulationModel()
				{
					assert(emu_model);
					return emu_model;
				}
			};

		}
	}
}

#endif	/* PERIPHERALMANAGER_H */

