/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

enum class GenCNodeType {
	STRING,
	INT,
	FLOAT,
	DOUBLE,

	ROOT,

	DefinitionList,

	Action,

	Typename,
	Constant,
	Type,
	Annotation,
	Vector,
	Reference,
	Struct,

	Helper,
	HelperParenList,
	HelperParen,
	HelperScope,

	HelperAttributeList,

	Execute,

	Body,

	Case,
	Default,
	Break,
	Continue,
	Raise,
	Return,
	While,
	Do,
	For,
	If,
	Switch,

	Declare,
	Call,
	ParamList,
	Binary,
	Variable,
	Cast,
	BitCast,

	Prefix,
	Postfix,
	Ternary,
	Member,
	VectorElement,
	BitExtract
};

static std::ostream &operator<<(std::ostream &os, GenCNodeType type)
{
#define HANDLE(x) case GenCNodeType::x: os << #x; break;
	switch(type) {
			HANDLE(STRING);
			HANDLE(INT);
			HANDLE(FLOAT);
			HANDLE(DOUBLE);
			HANDLE(ROOT);

			HANDLE(DefinitionList);
			HANDLE(Action);
			HANDLE(Typename);
			HANDLE(Constant);
			HANDLE(Type);
			HANDLE(Annotation);
			HANDLE(Vector);
			HANDLE(Reference);
			HANDLE(Struct);

			HANDLE(Helper);
			HANDLE(HelperParenList);
			HANDLE(HelperParen);
			HANDLE(HelperAttributeList);
			HANDLE(HelperScope);
			HANDLE(Execute);

			HANDLE(Case);
			HANDLE(Default);
			HANDLE(Break);
			HANDLE(Continue);
			HANDLE(Raise);
			HANDLE(Return);
			HANDLE(While);
			HANDLE(Do);
			HANDLE(For);
			HANDLE(If);
			HANDLE(Switch);

			HANDLE(Body);

			HANDLE(Declare);
			HANDLE(Call);
			HANDLE(ParamList);
			HANDLE(Binary);
			HANDLE(Variable);
			HANDLE(Cast);
			HANDLE(BitCast);

			HANDLE(Prefix);
			HANDLE(Postfix);
			HANDLE(Ternary);
			HANDLE(Member);
			HANDLE(VectorElement);
			HANDLE(BitExtract);

		default:
			throw std::logic_error("Unknown nodetype: " + std::to_string(static_cast<int>(type)));
	}
}

