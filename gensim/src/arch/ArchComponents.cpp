/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <string>

#include <stddef.h>
#include <stdint.h>

#include "arch/ArchComponents.h"

namespace gensim
{
	namespace arch
	{

		MemoryDescription::MemoryDescription(char *_name, uint32_t _size_bytes)
		{
			Name = _name;
			size_bytes = _size_bytes;
		}

		MemoryDescription::~MemoryDescription() {}

		RegBankDescription::~RegBankDescription() {}

		VectorRegBankDescription::~VectorRegBankDescription() {}
	}
}
