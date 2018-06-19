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
DeclareChildLogContext(LogZImage, LogLoader, "ZImage");

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>

using namespace archsim::abi::loader;

ZImageBinaryLoader::ZImageBinaryLoader(EmulationModel &emulation_model, Address base_addr, std::string symbol_map)
	: BinaryLoader(emulation_model, true), _base_addr(base_addr), _symbol_map(symbol_map) {}

ZImageBinaryLoader::ZImageBinaryLoader(EmulationModel &emulation_model, Address base_addr)
	: BinaryLoader(emulation_model, false), _base_addr(base_addr), _symbol_map("") {}

ZImageBinaryLoader::~ZImageBinaryLoader() {}

struct zimage_header {
	uint32_t code[9];
	uint32_t magic_number;
	uint32_t image_start;
	uint32_t image_end;
};

bool ZImageBinaryLoader::LoadSymbolMap(std::string map_file)
{
	std::ifstream symbols(map_file);

	std::string raw_line;
	while (std::getline(symbols, raw_line)) {
		std::stringstream raw_line_stream(raw_line);
		std::string addr_part;
		std::string type_part;
		std::string name_part;

		std::getline(raw_line_stream, addr_part, ' ');
		std::getline(raw_line_stream, type_part, ' ');
		std::getline(raw_line_stream, name_part);

		if (addr_part.size() == 0)
			continue;

		_emulation_model.AddSymbol(Address(std::stol(addr_part, 0, 16)), 0x1000, name_part, FunctionSymbol);
	}

	return true;
}

bool ZImageBinaryLoader::ProcessBinary(bool load_symbols)
{
	// Load symbols from the map file (if we have to, and we can)
	if (load_symbols && _symbol_map != "") {
		if (!LoadSymbolMap(_symbol_map))
			return false;
	}

	// Check ZImage magic number
	struct zimage_header *hdr = (struct zimage_header *)(_binary_data);
	if (hdr->magic_number != 0x016F2818) {
		LC_WARNING(LogZImage) << "[ZIMAGE] Attempted to load a non-zImage as a zImage binary";
		return false;
	}

	_entry_point = _base_addr;

	return _emulation_model.GetMemoryModel().Poke(_base_addr, (uint8_t *)_binary_data, _binary_size) == 0;
}
