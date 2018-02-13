#include "abi/devices/generic/block/COWBlockDevice.h"

using namespace archsim::abi::devices::generic::block;

COWBlockDevice::COWBlockDevice() : backing(NULL), snapshot(NULL)
{

}

COWBlockDevice::~COWBlockDevice()
{

}

bool COWBlockDevice::Open(BlockDevice& backing, BlockDevice& snapshot)
{
	assert(this->backing == NULL && this->snapshot == NULL);

	assert(backing.IsReadOnly());

	this->backing = &backing;
	this->snapshot = &snapshot;

	return true;
}

void COWBlockDevice::Close()
{
	assert(backing != NULL && snapshot != NULL);

	backing = NULL;
	snapshot = NULL;
}

bool COWBlockDevice::IsReadOnly() const
{
	return snapshot->IsReadOnly();
}

uint64_t COWBlockDevice::GetBlockCount() const
{
	return backing->GetBlockCount();
}

bool COWBlockDevice::ReadBlock(uint64_t block_idx, uint8_t* buffer)
{
	assert(false);
	return false;
}

bool COWBlockDevice::ReadBlocks(uint64_t block_idx, uint32_t count, uint8_t* buffer)
{
	assert(false);
	return false;
}

bool COWBlockDevice::WriteBlock(uint64_t block_idx, const uint8_t* buffer)
{
	assert(false);
	return false;
}

bool COWBlockDevice::WriteBlocks(uint64_t block_idx, uint32_t count, const uint8_t* buffer)
{
	assert(false);
	return false;
}
