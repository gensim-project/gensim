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

	class GenCSSAScanner : public yyFlexLexer
	{
	public:
		GenCSSAScanner(std::istream *input) : yyFlexLexer(input) {}

		virtual int yylex(typename GenCSSAParser::semantic_type *const lval, typename GenCSSAParser::location_type *location);

	private:
		typename GenCSSAParser::semantic_type *yylval = nullptr;
		//typename GenCParser::location_type *loc = nullptr;
	};

}