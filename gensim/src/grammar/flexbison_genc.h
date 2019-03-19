/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "genC.tabs.h"

namespace GenC
{

	class GenCScanner : public yyFlexLexer
	{
	public:
		GenCScanner(std::istream *input) : yyFlexLexer(input) {}

		virtual int yylex(typename GenCParser::semantic_type *const lval, typename GenCParser::location_type *location);

	private:
		typename GenCParser::semantic_type *yylval = nullptr;
		//typename GenCParser::location_type *loc = nullptr;
	};

}