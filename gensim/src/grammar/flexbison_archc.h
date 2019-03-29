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

	class ArchCScanner : public LexerTemplate<yyFlexLexer, ArchCParser>
	{
	public:
		ArchCScanner(std::istream *input) : BaseT(input) {}

		virtual int yylex(typename BaseParser::semantic_type *const lval, typename BaseParser::location_type *location) override;
	};

}
