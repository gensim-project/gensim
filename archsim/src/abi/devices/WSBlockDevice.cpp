/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/WSBlockDevice.h"
#include "abi/devices/Component.h"
#include "abi/memory/MemoryModel.h"
#include <cmath>

using namespace archsim::abi;
using namespace archsim::abi::devices;
using namespace archsim::abi::devices::generic::block;

// Version/Magic number = 1
#define WS_BLK_OFFSET_MAGIC 0x0

// log2 of sector size (512 byte sector = 9)
#define WS_BLK_OFFSET_BSIZE 0x4

// total number of sectors on disk
#define WS_BLK_OFFSET_BCOUNT 0x8

// index of block to be accessed
#define WS_BLK_OFFSET_BINDEX 0xc

// count of blocks to be accessed
#define WS_BLK_OFFSET_BOPCOUNT 0x10

// perform synchronous operation: 1 = write to host, 2 = read from host
#define WS_BLK_OFFSET_BOP 0x14

#define WS_BLK_OP_WTH 1
#define WS_BLK_OP_RFH 2

ComponentDescriptor ws_descriptor ("WSBlockDevice", {});
WSBlockDevice::WSBlockDevice(EmulationModel& model, archsim::Address base_addr, BlockDevice& bdev) : MemoryComponent(model, base_addr, 0x1000), bdev(bdev), Component(ws_descriptor)
{

}

WSBlockDevice::~WSBlockDevice()
{

}


bool WSBlockDevice::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	offset &= 0xfff;
	switch(offset) {
		case WS_BLK_OFFSET_MAGIC:
			data = 0;
			break;
		case WS_BLK_OFFSET_BSIZE:
			data = log2(bdev.GetBlockSize());
			break;
		case WS_BLK_OFFSET_BCOUNT:
			data = bdev.GetBlockCount();
			break;
		case WS_BLK_OFFSET_BINDEX:
			data = bindex;
			break;
		case WS_BLK_OFFSET_BOPCOUNT:
			data = bopcount;
			break;
	}
	return true;
}

bool WSBlockDevice::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	offset &= 0xfff;
	switch(offset) {
		case WS_BLK_OFFSET_BINDEX:
			bindex = data;
			break;
		case WS_BLK_OFFSET_BOPCOUNT:
			bopcount = data;
			break;
		case WS_BLK_OFFSET_BOP:
			HandleOp(data);
			break;
	}
	return true;
}

bool WSBlockDevice::HandleOp(uint32_t op_idx)
{
	switch(op_idx) {
		case WS_BLK_OP_WTH:
			HandleWTH();
			break;
		case WS_BLK_OP_RFH:
			HandleRFH();
			break;
		default:
			assert(false);
	}
	return true;
}

bool WSBlockDevice::HandleRFH()
{
	host_addr_t buffer;
	GetParentModel().GetMemoryModel().LockRegion(GetBaseAddress()+0x1000, bopcount * bdev.GetBlockSize(), buffer);
	bdev.ReadBlocks(bindex, bopcount, (uint8_t*)buffer);
	return true;
}

bool WSBlockDevice::HandleWTH()
{
	host_addr_t buffer;
	GetParentModel().GetMemoryModel().LockRegion(GetBaseAddress()+0x1000, bopcount * bdev.GetBlockSize(), buffer);
	bdev.WriteBlocks(bindex, bopcount, (uint8_t*)buffer);
	return true;
}

