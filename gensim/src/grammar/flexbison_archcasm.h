/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer ArchCAsmFlexLexer
#include <FlexLexer.h>
#endif

#include "archCAsm.tabs.h"

namespace ArchCAsm
{
	using AstNode = astnode<ArchCAsmNodeType>;

	class ArchCAsmScanner : public LexerTemplate<yyFlexLexer, ArchCAsmParser>
	{
	public:
		ArchCAsmScanner(std::istream *input) : BaseT(input) {}

		virtual int yylex(typename ArchCAsmParser::semantic_type *const lval, typename ArchCAsmParser::location_type *location) override;
	};

}