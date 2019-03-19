/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

template<typename BisonParser> class FlexScanner : public yyFlexLexer
{
public:
	FlexScanner(std::istream &input) : yyFlexLexer(input) {}

	virtual int yylex(typename BisonParser::semantic_type *const lval, typename BisonParser::location_type *location);

private:
	typename BisonParser::semantic_type *yylval = nullptr;
	typename BisonParser::location_type *loc = nullptr;
};
