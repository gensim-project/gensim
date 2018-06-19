/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/block/BlockDevice.h"
#include "util/LogContext.h"

UseLogContext(LogDevice);
DeclareChildLogContext(LogBlockDevice, LogDevice, "Block");

using namespace archsim::abi::devices::generic::block;

BlockDevice::BlockDevice()
{

}

BlockDevice::~BlockDevice()
{

}
