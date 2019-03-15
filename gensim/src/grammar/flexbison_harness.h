/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

enum class GenCNodeType {
	STRING,
	INT,

	ROOT,

	DefinitionList,

	Action,

	Typename,
	Constant,
	Type,
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

	Unary,
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
			HANDLE(ROOT);

			HANDLE(DefinitionList);
			HANDLE(Action);
			HANDLE(Typename);
			HANDLE(Constant);
			HANDLE(Type);
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

			HANDLE(Unary);
			HANDLE(Member);
			HANDLE(VectorElement);
			HANDLE(BitExtract);

		default:
			throw std::logic_error("Unknown nodetype: " + std::to_string(static_cast<int>(type)));
	}
}

template<typename nodeclass> class astnode
{
public:
	using ThisT = astnode<nodeclass>;

	astnode(nodeclass clazz) : class_(clazz) {}
	astnode(const char *data) : class_(nodeclass::STRING), strdata_(strdup(data)) {}
	astnode(uint64_t data) : class_(nodeclass::INT), intdata_(data) {}

	~astnode()
	{
		for(auto i : children_) {
			delete i;
		}
	}

	void AddChild(astnode *newchild)
	{
		children_.push_back(newchild);
	}
	void AddChildren(std::initializer_list<astnode*> newchildren)
	{
		children_.insert(children_.end(), newchildren.begin(), newchildren.end());
	}

	void Dump(int depth=0)
	{
		for(int i = 0; i < depth; ++i) {
			std::cout << "\t";
		}
		std::cout << class_;
		switch(class_) {
			case GenCNodeType::STRING:
				std::cout << " = " << strdata_;
				break;
			case GenCNodeType::INT:
				std::cout << " = " << intdata_;
				break;
		}

		if(children_.size()) {
			std::cout << "{\n";
			for(auto child : children_) {
				child->Dump(depth+1);
			}
			for(int i = 0; i < depth; ++i) {
				std::cout << "\t";
			}
			std::cout << "}";
		}
		std::cout << "\n";
	}

private:
	nodeclass class_;

	std::vector<astnode*> children_;

	char *strdata_;
	uint64_t intdata_;
};

template <typename nodeclass> astnode<nodeclass> *CreateNode(nodeclass clazz)
{
	return new astnode<nodeclass>(clazz);
}
template <typename nodeclass> astnode<nodeclass> *CreateNode(nodeclass clazz, const std::initializer_list<astnode<nodeclass>*> &children)
{
	auto rval = new astnode<nodeclass>(clazz);
	rval->AddChildren(children);
	return rval;
}

template <typename nodeclass> astnode<nodeclass> *CreateStrNode(const char *data)
{
	return new astnode<nodeclass>(data);
}
template <typename nodeclass> astnode<nodeclass> *CreateIntNode(uint64_t data)
{
	return new astnode<nodeclass>(data);
}