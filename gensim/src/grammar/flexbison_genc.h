/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer GenCFlexLexer
#include <FlexLexer.h>
#endif

#include "genC.tabs.h"

namespace GenC
{
	using AstNode = astnode<GenCNodeType>;

	class GenCScanner : public LexerTemplate<yyFlexLexer, GenCParser>
	{
	public:
		GenCScanner(std::istream *input) : BaseT(input) {}

		virtual int yylex(typename GenCParser::semantic_type *const lval, typename GenCParser::location_type *location) override;
	};

}