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

	IsaFile,
	FetchSize,
	Include,
	Instruction,
	Field,
	PC,
	Features,
	AsmMap,
	BehavioursList,

	IsaCtor,
	BehavioursFile,
	DecodesFile,
	ExecutesFile,

	Decoder,
	Assembler,
	Behaviour,
	EndOfBlock,
	JumpVariable,
	JumpFixed
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
			HANDLE(AcIsa);
			HANDLE(AcEndianness);
			HANDLE(SetFeature);
			HANDLE(AcTypename);
			HANDLE(AcConstant);

			HANDLE(IsaFile);
			HANDLE(FetchSize);
			HANDLE(Include);
			HANDLE(Instruction);
			HANDLE(Field);
			HANDLE(PC);
			HANDLE(Features);
			HANDLE(AsmMap);
			HANDLE(BehavioursList);

			HANDLE(IsaCtor);
			HANDLE(BehavioursFile);
			HANDLE(DecodesFile);
			HANDLE(ExecutesFile);

			HANDLE(Decoder);
			HANDLE(Assembler);
			HANDLE(Behaviour);
			HANDLE(EndOfBlock);
			HANDLE(JumpVariable);
			HANDLE(JumpFixed);

#undef HANDLE
		default:
			throw std::logic_error("Unknown nodetype: " + std::to_string(static_cast<int>(type)));
	}

	return os;
}

