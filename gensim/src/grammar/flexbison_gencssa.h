/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer GenCFlexLexer
#include <FlexLexer.h>
#endif

#include "genCSSA.tabs.h"

namespace GenCSSA
{
	using AstNode = astnode<GenCSSANodeType>;

	class GenCSSAScanner : public LexerTemplate<yyFlexLexer, GenCSSAParser>
	{
	public:
		GenCSSAScanner(std::istream *input) : BaseT(input) {}

		virtual int yylex(typename BaseParser::semantic_type *const lval, typename BaseParser::location_type *location);
	};

}