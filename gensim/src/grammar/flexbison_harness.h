/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

// Flex/bison creates a separate position/location data class for each grammar.
// In order to avoid dealing with this, define a single position and location
// class which we can convert these into.
class position_data
{
public:
	position_data(const std::string &filename = "(Unknown File)", int line = 0, int column = 0) : Filename(filename), Line(line), Column(column) {}

	std::string Filename;
	int Line;
	int Column;
};

class location_data
{
public:
	location_data() {};
	location_data(const position_data &begin, const position_data &end);

	position_data begin, end;
};

template<typename stream_type> static stream_type &operator<<(stream_type &str, const location_data &location)
{
	str << location.begin.Filename << ":" << location.begin.Line;
	return str;
}

// Flex/bison creates a completely separate base class for each lexer which
// makes it difficult to reason about and abstract away these classes. So,
// use a templated class to join these together into a common interface and
// common implementations for helper methods and diagnostics.
template<typename BaseLexerT, typename BaseParserT> class LexerTemplate : public BaseLexerT
{
public:
	using BaseT = LexerTemplate<BaseLexerT, BaseParserT>;
	using BaseLexer = BaseLexerT;
	using BaseParser = BaseParserT;

	LexerTemplate(std::istream *input) : BaseLexer(input) {}

	virtual void LexerError( const char* msg ) override
	{
		std::cerr << msg;
		throw std::logic_error(msg);
	}
	virtual int yylex(typename BaseParser::semantic_type *const lval, typename BaseParser::location_type *location) = 0;

protected:
	typename BaseParser::semantic_type *yylval;
};

template<typename nodeclass> class astnode
{
public:
	using ThisT = astnode<nodeclass>;
	using ChildListT = std::vector<ThisT*>;

	astnode(nodeclass clazz) : class_(clazz) {}
	astnode(const char *data) : class_(nodeclass::STRING), strdata_(strdup(data)) {}
	astnode(uint64_t data) : class_(nodeclass::INT), intdata_(data) {}
	astnode(float data) : class_(nodeclass::FLOAT), floatdata_(data) {}
	astnode(double data) : class_(nodeclass::DOUBLE), doubledata_(data) {}

	astnode(const ThisT& other) : class_(other.class_)
	{
		switch(class_) {
			case nodeclass::INT:
				intdata_ = other.intdata_;
				break;
			case nodeclass::DOUBLE:
				doubledata_ = other.doubledata_;
				break;
			case nodeclass::FLOAT:
				floatdata_ = other.floatdata_;
				break;
			case nodeclass::STRING:
				strdata_ = strdup(other.strdata_);
				break;
			default :
				break;
		}

		for(auto child : other.children_) {
			children_.push_back(new ThisT(*child));
		}
	}

	~astnode()
	{
		for(auto i : children_) {
			delete i;
		}
	}

	ThisT &operator[](int i)
	{
		return *GetChildren().at(i);
	}
	const ThisT &operator[](int i) const
	{
		return *GetChildren().at(i);
	}

	void SetLocation(const location_data &loc)
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

	nodeclass GetType() const
	{
		return class_;
	}
	typename ChildListT::iterator begin()
	{
		return children_.begin();
	}
	typename ChildListT::iterator end()
	{
		return children_.end();
	}
	typename ChildListT::const_iterator begin() const
	{
		return children_.begin();
	}
	typename ChildListT::const_iterator end() const
	{
		return children_.end();
	}
	ChildListT &GetChildren()
	{
		return children_;
	}
	const ChildListT &GetChildren() const
	{
		return children_;
	}

	const location_data &GetLocation() const
	{
		return location_;
	}

	void Dump(int depth=0) const
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

	const std::string GetString() const
	{
		assert(class_ == nodeclass::STRING);
		return strdata_;
	}
	uint64_t GetInt() const
	{
		assert(class_ == nodeclass::INT);
		return intdata_;
	}
	float GetFloat() const
	{
		assert(class_ == nodeclass::FLOAT);
		return floatdata_;
	}
	double GetDouble() const
	{
		assert(class_ == nodeclass::DOUBLE);
		return doubledata_;
	}

private:
	nodeclass class_;

	ChildListT children_;

	char *strdata_;
	uint64_t intdata_;
	float floatdata_;
	double doubledata_;

	location_data location_;
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
template <typename nodeclass> astnode<nodeclass> *CreateFloatNode(float data)
{
	return new astnode<nodeclass>(data);
}
template <typename nodeclass> astnode<nodeclass> *CreateDoubleNode(double data)
{
	return new astnode<nodeclass>(data);
}
