/*
 * File:   ArchComponents.cpp
 * Author: s0803652
 *
 * Created on 27 September 2011, 12:21
 */

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
