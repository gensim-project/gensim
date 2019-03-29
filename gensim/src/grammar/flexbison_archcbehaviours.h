/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <iostream>
#include <string>

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer ArchCFlexLexer
#include <FlexLexer.h>
#endif

#include "archCBehaviours.tabs.h"

namespace ArchCBehaviours
{
	using AstNode = astnode<ArchCBehavioursNodeType>;

	class ArchCBehavioursScanner : public LexerTemplate<yyFlexLexer, ArchCBehavioursParser>
	{
	public:
		ArchCBehavioursScanner(std::istream *input) : BaseT(input) {}

		virtual int yylex(typename ArchCBehavioursParser::semantic_type *const lval, typename ArchCBehavioursParser::location_type *location) override;
	};

}