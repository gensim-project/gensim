/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/Component.h"
#include "abi/devices/virtio/VirtIOBlock.h"
#include "abi/devices/generic/block/BlockDevice.h"
#include "util/LogContext.h"
#include "abi/devices/virtio/VirtQueue.h"
#include "abi/devices/IRQController.h"
#include <string.h>

UseLogContext(LogVirtIO);
DeclareChildLogContext(LogBlock, LogVirtIO, "Block");

using namespace archsim::abi::devices::virtio;
using namespace archsim::abi::devices;

static ComponentDescriptor virtioblock_descriptor ("VirtioBlock");
VirtIOBlock::VirtIOBlock(EmulationModel& parent_model, IRQLine& irq, Address base_address, std::string name, generic::block::BlockDevice& bdev)
	: VirtIO(parent_model, irq, base_address, 0x1000, name, 1, 2, 1), bdev(bdev), Component(virtioblock_descriptor)
{
	bzero(&config, sizeof(config));
	config.capacity = bdev.GetBlockCount();
	config.block_size = bdev.GetBlockSize();

	config.seg_max = 128-2;
	config.wce = 1;

	HostFeatures.Set(1 << 6);

	LC_DEBUG1(LogBlock) << "Configured block device with capacity " << std::hex << config.capacity;
}

VirtIOBlock::~VirtIOBlock()
{

}

void VirtIOBlock::ResetDevice()
{

}

void VirtIOBlock::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	if(reg == HostFeaturesSel) {
		if(value == 0) {
			HostFeatures.Set(0x00000040);
		} else {
			HostFeatures.Set(0);
		}
	} else {
		VirtIO::WriteRegister(reg, value);
	}
}


void VirtIOBlock::ProcessEvent(VirtIOQueueEvent* evt_)
{
	VirtIOQueueEvent &evt = *evt_;
	LC_DEBUG1(LogBlock) << "Processing Event";

	if (evt.read_buffers.size() == 0 && evt.write_buffers.size() == 0)
		return;

	assert(evt.read_buffers.size() > 0);
	assert(evt.write_buffers.size() > 0);

	if (evt.read_buffers.front().size < sizeof(struct virtio_blk_req)) {
		LC_DEBUG1(LogBlock) << "Discarding event with invalid header";
		return;
	}

	struct virtio_blk_req *req = (struct virtio_blk_req *)evt.read_buffers.front().data;

	uint8_t *status = (uint8_t *)evt.write_buffers.back().data;
	switch (req->type) {
		case 0: // Read
			assert(evt.write_buffers.size() == 2);

			if (HandleRead(req->sector, (uint8_t *)evt.write_buffers.front().data, evt.write_buffers.front().size)) {
				*status = 0;
			} else {
				*status = 1;
			}

			evt.response_size = evt.write_buffers.front().size + 1;
			break;

		case 1: // Write
			assert(evt.read_buffers.size() == 2);

			if (HandleWrite(req->sector, (uint8_t *)evt.read_buffers.back().data, evt.read_buffers.back().size)) {
				*status = 0;
			} else {
				*status = 1;
			}

			evt.response_size = 1;
			break;
		case 8: { // Get ID
			assert(evt.write_buffers.size() == 2);

			char *serial_number = (char *)evt.write_buffers.front().data;
			strncpy(serial_number, "virtio", evt.write_buffers.front().size);

			evt.response_size = 1 + 7;
			*status = 0;

			break;
		}
		default:
			LC_ERROR(LogBlock) << "Rejecting event with unsupported type " << (uint32_t)req->type;

			*status = 2;
			evt.response_size = 1;
			break;
	}

	evt.owner.Push(evt.Index(), evt.response_size);
	LC_DEBUG1(LogVirtIO) << "[" << GetName() << "] Pushed a descriptor chain head " << std::dec << evt.Index() << ", length=" << evt.response_size;

	AssertInterrupt(1);

	// We're done with the event descriptor so delete it
	delete evt_;
}

uint8_t *VirtIOBlock::GetConfigArea() const
{
	return (uint8_t *)&config;
}

uint32_t VirtIOBlock::GetConfigAreaSize() const
{
	return sizeof(config);
}

bool VirtIOBlock::HandleRead(uint64_t sector, uint8_t* buffer, uint32_t len)
{
	LC_DEBUG1(LogBlock) << "Handling read sector=" << std::hex << sector << ", len=" << std::dec << len;
	return bdev.ReadBlocks(sector, len / bdev.GetBlockSize(), buffer);
}

bool VirtIOBlock::HandleWrite(uint64_t sector, uint8_t* buffer, uint32_t len)
{
	LC_DEBUG1(LogBlock) << "Handling write sector=" << std::hex << sector << ", len=" << std::dec << len;
	return bdev.WriteBlocks(sector, len / bdev.GetBlockSize(), buffer);
}
