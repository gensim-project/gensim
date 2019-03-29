/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

enum class GenCSSANodeType {
	STRING,
	INT,
	FLOAT,
	DOUBLE,

	ROOT,
	List,

	Context,
	Action,
	Id,
	BasicType,
	StructType,
	VectorType,
	ReferenceType,
	AttributeNoinline,
	AttributeHelper,
	AttributeGlobal,
	AttributeExport,

	Block,
	Statement,
	StatementBinary,
	StatementConstant,
	StatementCast,
	StatementDevRead,
	StatementDevWrite,
	StatementRegRead,
	StatementRegWrite,
	StatementBankRegRead,
	StatementBankRegWrite,
	StatementIntrinsic,
	StatementPhi,
	StatementReturn,
	StatementReturnVoid,
	StatementRaise,
	StatementSelect,
	StatementSwitch,
	StatementUnary,
	StatementStructMember,
	StatementVInsert,
	StatementVExtract,
	StatementIf,
	StatementJump,
	StatementMemRead,
	StatementMemWrite,
	StatementVariableRead,
	StatementVariableWrite,
	StatementCall

};

static std::ostream &operator<<(std::ostream &os, GenCSSANodeType type)
{
#define HANDLE(x) case GenCSSANodeType::x: os << #x; break;
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

