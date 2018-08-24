/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/loader/BinaryLoader.h"
#include "abi/EmulationModel.h"
#include "util/LogContext.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

using namespace archsim::abi::loader;

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogLoader, LogEmulationModel, "Loader");

BinaryLoader::BinaryLoader(EmulationModel &emulation_model, bool load_symbols) : _emulation_model(emulation_model), _load_symbols(load_symbols) {}

BinaryLoader::~BinaryLoader()
{
	if (_binary_data != NULL) {
		munmap((void *)_binary_data, _binary_size);
		_binary_data = NULL;
	}
}

bool BinaryLoader::LoadBinary(std::string filename)
{
	_binary_filename = filename;

	int fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0) return false;

	struct stat st;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return false;
	}

	_binary_size = st.st_size;
	_binary_data = (const char *)mmap(NULL, _binary_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	if (_binary_data == MAP_FAILED) return false;

	return ProcessBinary(_load_symbols);
}

archsim::Address BinaryLoader::GetEntryPoint()
{
	return _entry_point;
}
