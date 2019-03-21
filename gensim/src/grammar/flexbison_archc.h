/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer ArchCFlexLexer
#include <FlexLexer.h>
#endif

#include "archC.tabs.h"

namespace ArchC
{
	using AstNode = astnode<ArchCNodeType>;

	class ArchCScanner : public yyFlexLexer
	{
	public:
		ArchCScanner(std::istream *input) : yyFlexLexer(input) {}

		virtual int yylex(typename ArchCParser::semantic_type *const lval, typename ArchCParser::location_type *location);

	private:
		typename ArchCParser::semantic_type *yylval = nullptr;
	};

}