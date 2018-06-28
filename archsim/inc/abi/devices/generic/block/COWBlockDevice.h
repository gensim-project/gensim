/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   COWBlockDevice.h
 * Author: s0457958
 *
 * Created on 29 October 2014, 12:04
 */

#ifndef COWBLOCKDEVICE_H
#define	COWBLOCKDEVICE_H

#include "abi/devices/generic/block/BlockDevice.h"

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
					class COWBlockDevice : public BlockDevice
					{
					public:
						COWBlockDevice();
						~COWBlockDevice();

						uint64_t GetBlockCount() const override;
						bool IsReadOnly() const override;

						bool ReadBlock(uint64_t block_idx, uint8_t* buffer) override;
						bool ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t* buffer) override;
						bool WriteBlock(uint64_t block_idx, const uint8_t* buffer) override;
						bool WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t* buffer) override;

						bool Open(BlockDevice& backing, BlockDevice& snapshot);
						void Close();

					private:
						BlockDevice *backing, *snapshot;
					};
				}
			}
		}
	}
}

#endif	/* COWBLOCKDEVICE_H */
