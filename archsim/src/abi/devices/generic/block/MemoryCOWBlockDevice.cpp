/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "abi/devices/generic/block/MemoryCOWBlockDevice.h"
#include "util/LogContext.h"

#include <cstring>
#include <map>

UseLogContext(LogBlock);
DeclareChildLogContext(LogCOWBlock, LogBlock, "COW");

using namespace archsim::abi::devices::generic::block;

MemoryCOWBlockDevice::MemoryCOWBlockDevice() : _backing(nullptr)
{

}

MemoryCOWBlockDevice::~MemoryCOWBlockDevice()
{
	if(_backing) Close();
}

uint64_t MemoryCOWBlockDevice::GetBlockCount() const
{
	return _backing->GetBlockCount();
}

uint64_t MemoryCOWBlockDevice::GetBlockSize() const
{
	return _backing->GetBlockSize();
}

bool MemoryCOWBlockDevice::IsReadOnly() const
{
	return _backing->IsReadOnly();
}

bool MemoryCOWBlockDevice::ReadBlock(uint64_t block_idx, uint8_t* buffer)
{
	assert(_backing);
	auto snapshot_block = _snapshot_data.find(block_idx);

	LC_DEBUG1(LogCOWBlock) << "Reading block " << block_idx;

	if(snapshot_block == _snapshot_data.end()) {
		LC_DEBUG1(LogCOWBlock) << "Reading block " << block_idx << " from backing device";
		return _backing->ReadBlock(block_idx, buffer);
	} else {
		LC_DEBUG1(LogCOWBlock) << "Reading block " << block_idx << " from cache";
		memcpy(buffer, snapshot_block->second, GetBlockSize());
		return true;
	}
}

bool MemoryCOWBlockDevice::ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t* buffer)
{
	for(uint32_t i = 0; i < count; ++i) {
		if(!ReadBlock(block_idx+i, buffer + (GetBlockSize()*i))) return false;
	}
	return true;
}

bool MemoryCOWBlockDevice::WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t* buffer)
{
	for(uint32_t i = 0; i < count; ++i) {
		if(!WriteBlock(block_idx+i, buffer + (GetBlockSize()*i))) return false;
	}
	return true;
}

bool MemoryCOWBlockDevice::WriteBlock(uint64_t block_idx, const uint8_t* buffer)
{
	assert(_backing);
	LC_DEBUG1(LogCOWBlock) << "Writing block " << block_idx;

	void *&ptr = _snapshot_data[block_idx];
	if(ptr == nullptr) {
		LC_DEBUG2(LogCOWBlock) << "Allocating storage for block " << block_idx;
		ptr = malloc(GetBlockSize());
	}

	memcpy(ptr, buffer, GetBlockSize());
	return true;
}

bool MemoryCOWBlockDevice::Open(BlockDevice& backing)
{
	assert(!_backing);
	_backing = &backing;
	return true;
}

void MemoryCOWBlockDevice::Close()
{
	assert(_backing);
	for(auto i : _snapshot_data) free(i.second);
	_snapshot_data.clear();
	_backing = nullptr;
}
