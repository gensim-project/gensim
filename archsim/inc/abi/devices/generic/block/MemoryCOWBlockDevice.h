/*
 * File:   MemoryCOWBlockDevice.h
 * Author: harry
 *
 * Created on 18 April 2016, 11:32
 */

#ifndef MEMORYCOWBLOCKDEVICE_H
#define MEMORYCOWBLOCKDEVICE_H

#include "abi/devices/generic/block/BlockDevice.h"

#include <map>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				namespace block
				{
					class MemoryCOWBlockDevice : public BlockDevice
					{
					public:
						MemoryCOWBlockDevice();
						~MemoryCOWBlockDevice();

						uint64_t GetBlockCount() const override;
						uint64_t GetBlockSize() const override;
						bool IsReadOnly() const override;

						bool ReadBlock(uint64_t block_idx, uint8_t* buffer) override;
						bool ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t* buffer) override;
						bool WriteBlock(uint64_t block_idx, const uint8_t* buffer) override;
						bool WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t* buffer) override;

						bool Open(BlockDevice& backing);
						void Close();
						bool IsOpen() const override
						{
							return _backing != nullptr;
						}


					private:
						BlockDevice *_backing;
						std::map<uint64_t, void*> _snapshot_data;
					};
				}
			}
		}
	}
}

#endif /* MEMORYCOWBLOCKDEVICE_H */

