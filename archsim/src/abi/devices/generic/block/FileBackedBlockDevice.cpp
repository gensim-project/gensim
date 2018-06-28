/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/block/FileBackedBlockDevice.h"
#include "util/LogContext.h"

UseLogContext(LogBlockDevice);

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace archsim::abi::devices::generic::block;

FileBackedBlockDevice::FileBackedBlockDevice() : BlockDevice(), file_data(NULL), use_mmap(false), file_descr(0)
{

}

FileBackedBlockDevice::~FileBackedBlockDevice()
{
	assert(file_data == NULL);
}

bool FileBackedBlockDevice::Open(std::string filename, bool read_only)
{
	assert(file_data == NULL);

	cache.reset(new BlockCache());

	LC_DEBUG1(LogBlockDevice) << "Opening file " << filename;

	this->read_only = read_only;

	int fd = open(filename.c_str(), this->read_only ? O_RDONLY : O_RDWR);
	if (fd < 0) {
		LC_ERROR(LogBlockDevice) << "Failed to open file '" << filename << "'";
		return false;
	}

	struct stat st;
	if (fstat(fd, &st)) {
		LC_ERROR(LogBlockDevice) << "Failed to stat file";
		close(fd);
		return false;
	}

	LC_DEBUG1(LogBlockDevice) << "File size: " << st.st_size;

	file_size = st.st_size;

	if (use_mmap) {
		file_data = mmap(NULL, file_size, PROT_READ | (this->read_only ? 0 : PROT_WRITE), MAP_PRIVATE, fd, 0);
		close(fd);

		if (file_data == MAP_FAILED) {
			LC_ERROR(LogBlockDevice) << "Failed to mmap file";
			return false;
		}
	} else {
		file_data = NULL;
		file_descr = fd;
	}

	block_count = file_size / GetBlockSize();
	if (file_size % GetBlockSize() != 0)
		block_count++;

	LC_DEBUG1(LogBlockDevice) << "File opened and mapped to: " << file_data << ", blocks=" << std::dec << block_count;

	return true;
}

void FileBackedBlockDevice::Close()
{
	if(file_data == NULL) {
		LC_WARNING(LogBlockDevice) << "Tried to close a file backed block device, but it wasn't open";
		return;
	}

	LC_DEBUG1(LogBlockDevice) << "Closing file";

	if (use_mmap) {
		munmap(file_data, file_size);
		file_data = NULL;
	} else {
		close(file_descr);
	}
}

uint64_t FileBackedBlockDevice::GetBlockCount() const
{
	return block_count;
}

uint64_t FileBackedBlockDevice::GetBlockSize() const
{
	return 512;
}

bool FileBackedBlockDevice::ReadBlock(uint64_t block_idx, uint8_t* buffer)
{
	if (block_idx >= block_count) {
		LC_WARNING(LogBlockDevice) << "Attempted to read block past end of device";
		return false;
	}

	reads.inc();

	LC_DEBUG1(LogBlockDevice) << "Reading Block " << block_idx;

	if (use_mmap) {
		void *data_ptr = GetDataPtr(block_idx);
		memcpy(buffer, data_ptr, GetBlockSize());
	} else {
		ssize_t actually_read;
		do {
			lseek64(file_descr, block_idx * GetBlockSize(), SEEK_SET);
			actually_read = read(file_descr, buffer, GetBlockSize());
		} while (actually_read != (ssize_t)GetBlockSize());
	}

	return true;
}

bool FileBackedBlockDevice::ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t* buffer)
{
	assert(count < block_count);

	if (block_idx >= block_count) {
		LC_WARNING(LogBlockDevice) << "Attempted to read blocks past end of device";
		return false;
	}

	if (block_idx > block_count - count) {
		count = (uint32_t)(block_count - block_idx);
	}

	reads.inc(count);

	LC_DEBUG1(LogBlockDevice) << "Reading " << count << " blocks " << block_idx;

	if (use_mmap) {
		void *data_ptr = GetDataPtr(block_idx);
		memcpy(buffer, data_ptr, GetBlockSize() * count);
	} else {
		for(uint32_t i = 0; i < count; ++i) {
			if(cache->HasBlock(block_idx + i)) {
				cache->ReadBlock(block_idx + i, buffer);
			} else {
				ReadBlock(block_idx+i, buffer);
				cache->WriteBlock(block_idx + i, buffer);
			}

			buffer += GetBlockSize();
		}
	}

	return true;
}

bool FileBackedBlockDevice::WriteBlock(uint64_t block_idx, const uint8_t* buffer)
{
	assert(!read_only);

	if (block_idx >= block_count) {
		LC_WARNING(LogBlockDevice) << "Attempted to write block past end of device";
		return false;
	}

	writes.inc();

	LC_DEBUG1(LogBlockDevice) << "Writing Block " << block_idx;

	if (use_mmap) {
		void *data_ptr = GetDataPtr(block_idx);
		memcpy(data_ptr, buffer, GetBlockSize());
	} else {
		ssize_t actually_written;
		do {
			lseek64(file_descr, block_idx * GetBlockSize(), SEEK_SET);
			actually_written = write(file_descr, buffer, GetBlockSize());
		} while (actually_written != (ssize_t)GetBlockSize());
	}

	return true;
}

bool FileBackedBlockDevice::WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t* buffer)
{
	assert(!read_only);
	assert(count < block_count);

	if (block_idx >= block_count) {
		LC_WARNING(LogBlockDevice) << "Attempted to write blocks past end of device";
		return false;
	}

	if (block_idx > block_count - count) {
		count = block_count - block_idx;
	}

	writes.inc(count);
	LC_DEBUG1(LogBlockDevice) << "Writing " << count << " blocks " << block_idx;

	if (use_mmap) {
		void *data_ptr = GetDataPtr(block_idx);
		memcpy(data_ptr, buffer, GetBlockSize() * count);
	} else {
		for(uint32_t i = 0; i < count; ++i) {
			cache->WriteBlock(block_idx+i, buffer);
			WriteBlock(block_idx+i, buffer);
			buffer += GetBlockSize();
		}
	}

	return true;
}
