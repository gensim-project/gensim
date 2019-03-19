/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

#include "location.hh"

template<typename nodeclass, typename locclass> class astnode
{
public:
	using ThisT = astnode<nodeclass, locclass>;

	astnode(nodeclass clazz) : class_(clazz) {}
	astnode(const char *data) : class_(nodeclass::STRING), strdata_(strdup(data)) {}
	astnode(uint64_t data) : class_(nodeclass::INT), intdata_(data) {}
	astnode(float data) : class_(nodeclass::FLOAT), floatdata_(data) {}
	astnode(double data) : class_(nodeclass::DOUBLE), doubledata_(data) {}

	~astnode()
	{
		for(auto i : children_) {
			delete i;
		}
	}

	void SetLocation(locclass loc)
	{
		location_ = loc;
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
			case nodeclass::STRING:
				std::cout << " = " << strdata_;
				break;
			case nodeclass::INT:
				std::cout << " = " << intdata_;
				break;
			case nodeclass::FLOAT:
				std::cout << " = " << floatdata_;
				break;
			case nodeclass::DOUBLE:
				std::cout << " = " << doubledata_;
				break;
		}

		std::cout << " (" << location_ << ")";

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
	float floatdata_;
	double doubledata_;

	locclass location_;
};

template <typename nodeclass, typename locclass> astnode<nodeclass, locclass> *CreateNode(nodeclass clazz)
{
	return new astnode<nodeclass, locclass>(clazz);
}
template <typename nodeclass, typename locclass> astnode<nodeclass, locclass> *CreateNode(nodeclass clazz, const std::initializer_list<astnode<nodeclass, locclass>*> &children)
{
	auto rval = new astnode<nodeclass, locclass>(clazz);
	rval->AddChildren(children);
	return rval;
}

template <typename nodeclass, typename locclass> astnode<nodeclass, locclass> *CreateStrNode(const char *data)
{
	return new astnode<nodeclass, locclass>(data);
}
template <typename nodeclass, typename locclass> astnode<nodeclass, locclass> *CreateIntNode(uint64_t data)
{
	return new astnode<nodeclass, locclass>(data);
}
template <typename nodeclass, typename locclass> astnode<nodeclass, locclass> *CreateFloatNode(float data)
{
	return new astnode<nodeclass, locclass>(data);
}
template <typename nodeclass, typename locclass> astnode<nodeclass, locclass> *CreateDoubleNode(double data)
{
	return new astnode<nodeclass, locclass>(data);
}