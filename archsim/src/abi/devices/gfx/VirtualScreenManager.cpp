/*
 * VirtualScreenManager.cpp
 *
 *  Created on: 21 Jul 2014
 *      Author: harry
 */


#include "abi/devices/gfx/VirtualScreenManager.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"


using namespace archsim::abi::devices::gfx;

VirtualScreenManagerBase::~VirtualScreenManagerBase() {}

UseLogContext(LogDevice)
DeclareChildLogContext(LogGfx, LogDevice, "GFX");
DeclareChildLogContext(LogVSM, LogGfx, "Manager");

DefineComponentType(VirtualScreenManagerBase);
