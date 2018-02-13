/*
 * File:   BlockDevice.h
 * Author: spink
 *
 * Created on 18 September 2014, 08:21
 */

#ifndef BLOCKDEVICE_H
#define	BLOCKDEVICE_H

#include "define.h"
#include "util/Counter.h"

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
					class BlockDevice
					{
					public:
						BlockDevice();
						virtual ~BlockDevice();

						virtual bool ReadBlock(uint64_t block_idx, uint8_t *buffer) = 0;
						virtual bool ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t *buffer) = 0;
						virtual bool WriteBlock(uint64_t block_idx, const uint8_t *buffer) = 0;
						virtual bool WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t *buffer) = 0;

						virtual uint64_t GetBlockSize() const = 0;
						virtual uint64_t GetBlockCount() const = 0;

						virtual bool IsReadOnly() const = 0;
						virtual bool IsOpen() const = 0;

						util::Counter64 reads;
						util::Counter64 writes;

					protected:
						inline uint64_t CalculateByteOffset(uint64_t block_idx) const
						{
							return GetBlockSize() * block_idx;
						}

					};
				}
			}
		}
	}
}

#endif	/* BLOCKDEVICE_H */

