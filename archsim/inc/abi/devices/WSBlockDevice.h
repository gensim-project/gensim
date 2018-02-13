/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   WSBlockDevice.h
 * Author: harry
 *
 * Created on 24 May 2016, 17:35
 */

#ifndef WSBLOCKDEVICE_H
#define WSBLOCKDEVICE_H

#include "abi/devices/Device.h"
#include "abi/devices/Component.h"
#include "abi/devices/generic/block/BlockDevice.h"
#include "abi/EmulationModel.h"
#include "abi/Address.h"

class WSBlockDevice : public archsim::abi::devices::MemoryComponent
{
public:
	WSBlockDevice(archsim::abi::EmulationModel& model, archsim::Address base_addr, archsim::abi::devices::generic::block::BlockDevice& bdev);
	~WSBlockDevice();

	bool Read(uint32_t offset, uint8_t size, uint32_t& data) override;
	bool Write(uint32_t offset, uint8_t size, uint32_t data) override;

private:
	bool HandleOp(uint32_t op_idx);
	bool HandleWTH();
	bool HandleRFH();

	archsim::abi::devices::generic::block::BlockDevice& bdev;

	uint32_t bindex, bopcount;
};

#endif /* WSBLOCKDEVICE_H */

