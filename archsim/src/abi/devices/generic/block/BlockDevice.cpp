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
