/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Device.h
 * Author: harry
 *
 * Created on 11 December 2013, 15:09
 */

#ifndef DEVICE_H
#define	DEVICE_H

#include "util/ComponentManager.h"

#include <cstdint>
#include <map>
#include <string>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class PeripheralManager;

			class Device
			{
			public:
				Device();
				virtual ~Device();

				virtual bool Initialise() = 0;

				virtual bool Read8(uint32_t address, uint8_t &data);
				virtual bool Write8(uint32_t address, uint8_t data);
				virtual bool Read16(uint32_t address, uint16_t &data);
				virtual bool Write16(uint32_t address, uint16_t data);
				virtual bool Read32(uint32_t address, uint32_t &data);
				virtual bool Read64(uint32_t address, uint64_t &data);
				virtual bool Write32(uint32_t address, uint32_t data);
				virtual bool Write64(uint32_t address, uint64_t data);
				virtual bool ReadBlock(uint32_t address, void *data, unsigned int length);
				virtual bool WriteBlock(uint32_t address, const void *data, unsigned int length);

				bool SetManager(PeripheralManager*);
				PeripheralManager *Manager;
			};

		}
	}
}

#endif	/* DEVICE_H */

