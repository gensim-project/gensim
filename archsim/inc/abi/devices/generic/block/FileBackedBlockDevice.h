/*
 * File:   FileBackedBlockDevice.h
 * Author: spink
 *
 * Created on 18 September 2014, 08:23
 */

#ifndef FILEBACKEDBLOCKDEVICE_H
#define	FILEBACKEDBLOCKDEVICE_H

#include "abi/devices/generic/block/BlockDevice.h"
#include "abi/devices/generic/block/BlockCache.h"

#include <memory>
#include <string>

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
					class FileBackedBlockDevice : public BlockDevice
					{
					public:
						FileBackedBlockDevice();
						~FileBackedBlockDevice();

						uint64_t GetBlockCount() const override;
						uint64_t GetBlockSize() const override;

						bool ReadBlock(uint64_t block_idx, uint8_t* buffer) override;
						bool ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t* buffer) override;
						bool WriteBlock(uint64_t block_idx, const uint8_t* buffer) override;
						bool WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t* buffer) override;

						bool Open(std::string filename, bool read_only = false);
						void Close();

						bool IsReadOnly() const override
						{
							return read_only;
						}

						bool IsOpen() const override
						{
							return file_data != NULL;
						}

					private:
						bool use_mmap;

						inline void *GetDataPtr(uint64_t block_idx) const
						{
							return (void *)((unsigned long)file_data + CalculateByteOffset(block_idx));
						}

						int file_descr;
						void *file_data;
						uint64_t file_size;
						uint64_t block_count;
						bool read_only;

						std::unique_ptr<BlockCache> cache;
					};
				}
			}
		}
	}
}

#endif	/* FILEBACKEDBLOCKDEVICE_H */

