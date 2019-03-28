/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

enum class ArchCBehavioursNodeType {
	STRING,
	INT,
	FLOAT,
	DOUBLE,

	ROOT,
	List,
	Constant,
	DecodeBehaviour,
	GenericBehaviour

};

static std::ostream &operator<<(std::ostream &os, ArchCBehavioursNodeType type)
{
#define HANDLE(x) case ArchCBehavioursNodeType::x: os << #x; break;
	switch(type) {
			HANDLE(STRING);
			HANDLE(INT);
			HANDLE(FLOAT);
			HANDLE(DOUBLE);
			HANDLE(ROOT);


#undef HANDLE
		default:
			throw std::logic_error("Unknown nodetype: " + std::to_string(static_cast<int>(type)));
	}

	return os;
}

