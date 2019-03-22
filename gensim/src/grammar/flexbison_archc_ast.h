/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

enum class ArchCNodeType {
	STRING,
	INT,
	FLOAT,
	DOUBLE,

	ROOT,

	Identifier,
	Arch,
	List,
	WordSize,
	Format,
	MemInterface,
	RegSpace,
	RegViewSlot,
	RegViewBank,

	ArchCtor,
	AcIsa,
	AcEndianness,
	SetFeature,
	AcTypename,
	AcConstant,
};

static std::ostream &operator<<(std::ostream &os, ArchCNodeType type)
{
#define HANDLE(x) case ArchCNodeType::x: os << #x; break;
	switch(type) {
			HANDLE(STRING);
			HANDLE(INT);
			HANDLE(FLOAT);
			HANDLE(DOUBLE);
			HANDLE(ROOT);

			HANDLE(Identifier);
			HANDLE(Arch);
			HANDLE(List);
			HANDLE(WordSize);
			HANDLE(Format);
			HANDLE(MemInterface);
			HANDLE(RegSpace);
			HANDLE(RegViewSlot);
			HANDLE(RegViewBank);

			HANDLE(ArchCtor);

#undef HANDLE
		default:
			throw std::logic_error("Unknown nodetype: " + std::to_string(static_cast<int>(type)));
	}

	return os;
}

