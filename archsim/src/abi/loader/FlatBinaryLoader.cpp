/*
 * FlagBinaryLoader.cpp
 *
 *  Created on: 8 Dec 2013
 *      Author: harry
 */

#include "abi/EmulationModel.h"
#include "abi/loader/BinaryLoader.h"
#include "abi/memory/MemoryModel.h"

#include "util/LogContext.h"

UseLogContext(LogLoader);
DeclareChildLogContext(LogFlatBinary, LogLoader, "FlatBinary");

using namespace archsim::abi::loader;

FlatBinaryLoader::FlatBinaryLoader(EmulationModel &emulation_model, Address base_addr) : BinaryLoader(emulation_model, false), _base_addr(base_addr) {}

FlatBinaryLoader::~FlatBinaryLoader() {}

bool FlatBinaryLoader::ProcessBinary(bool load_symbols)
{
	return _emulation_model.GetMemoryModel().Poke(_base_addr, (uint8_t *)_binary_data, _binary_size) == 0;
}
