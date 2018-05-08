#include "isa/HelperFnDescriptionParser.h"
#include "isa/HelperFnDescription.h"
#include "DiagnosticContext.h"

#include <sstream>

using namespace gensim;
using namespace gensim::isa;

HelperFnDescriptionLexer::HelperFnDescriptionLexer(std::istream& stream) : stream_(stream)
{
	PopChar();
	Pop();
}

const HelperFnDescriptionToken* HelperFnDescriptionLexer::Pop()
{
	const HelperFnDescriptionToken *old = current_;

	SkipWhitespace();

	if (PeekChar() == 0) {
		current_ = nullptr;
	} else if (isalpha(PeekChar())) {
		current_ = ReadIdentifier();
	} else if (isdigit(PeekChar())) {
		current_ = ReadNumber();
	} else if (PeekChar() == '(') {
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::LPAREN, "(");
		PopChar();
	} else if (PeekChar() == ')') {
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::RPAREN, ")");
		PopChar();
	} else if (PeekChar() == '[') {
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::LBRACKET, "[");
		PopChar();
	} else if (PeekChar() == ']') {
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::RBRACKET, "]");
		PopChar();
	} else if (PeekChar() == ',') {
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::COMMA, ",");
		PopChar();
	} else if (PeekChar() == '&') {
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::AMPERSAND, "&");
		PopChar();
	} else {
		char c[] = {PopChar(),0};
		current_ = new HelperFnDescriptionToken(HelperFnDescriptionToken::UNKNOWN, std::string(c));
	}

	return old;
}

const HelperFnDescriptionToken* HelperFnDescriptionLexer::ReadIdentifier()
{
	std::string ident;
	while (PeekChar() && (isalnum(PeekChar()) || PeekChar() == '_')) {
		ident += PopChar();
	}

	if (ident == "private") {
		return new HelperFnDescriptionToken(HelperFnDescriptionToken::PRIVATE, ident);
	} else if (ident == "internal") {
		return new HelperFnDescriptionToken(HelperFnDescriptionToken::INTERNAL, ident);
	} else if (ident == "public") {
		return new HelperFnDescriptionToken(HelperFnDescriptionToken::PUBLIC, ident);
	} else {
		return new HelperFnDescriptionToken(HelperFnDescriptionToken::IDENTIFIER, ident);
	}
}

const HelperFnDescriptionToken* HelperFnDescriptionLexer::ReadNumber()
{
	std::string number;
	while (PeekChar() && isdigit(PeekChar())) {
		number += PopChar();
	}

	return new HelperFnDescriptionToken(HelperFnDescriptionToken::NUMBER, number);
}

void HelperFnDescriptionLexer::SkipWhitespace()
{
	while (PeekChar() && isspace(PeekChar())) PopChar();
}

char HelperFnDescriptionLexer::PopChar()
{
	char old = current_char_;

	if (stream_.eof()) {
		current_char_ = 0;
	} else {
		current_char_ = stream_.get();
	}

	return old;
}

// type_name : IDENT
// type_spec : type_name | type_name LBRACKET INTEGER RBRACKET
// type : type_spec | type_spec AMP
// prototype : type IDENT LPAREN params RPAREN attributes
// params : | param | param ',' params
// param : type IDENT
// attributes : | IDENT attributes

HelperFnDescription *HelperFnDescriptionParser::Parse(const std::string& body)
{
	return ParsePrototype(body);
}

const HelperFnDescriptionToken* HelperFnDescriptionParser::Expect(HelperFnDescriptionToken::TokenKind kind)
{
	const auto token = Match(kind);
	if (token == nullptr) throw std::logic_error("Error: Expected " + std::to_string(kind) + ", got " + std::to_string(lexer_.Peek()->GetKind()));

	return token;
}

const HelperFnDescriptionToken* HelperFnDescriptionParser::Match(HelperFnDescriptionToken::TokenKind kind)
{
	const auto token = lexer_.Peek();
	if (token->GetKind() == kind) return lexer_.Pop();

	return nullptr;
}

const HelperFnDescriptionToken* HelperFnDescriptionParser::Is(HelperFnDescriptionToken::TokenKind kind)
{
	const auto token = lexer_.Peek();
	if (token->GetKind() == kind) return lexer_.Peek();

	return nullptr;
}

HelperFnDescription* HelperFnDescriptionParser::ParsePrototype(const std::string& body)
{
	auto descr = new HelperFnDescription(body);

	descr->return_type = ParseType();
	descr->name = Expect(HelperFnDescriptionToken::IDENTIFIER)->GetValue();
	Expect(HelperFnDescriptionToken::LPAREN);

	auto params = ParseParams();
	for (const auto& param : params) {
		HelperFnParamDescription param_desc;
		param_desc.name = param.second;
		param_desc.type = param.first;

		descr->params.push_back(param_desc);
	}

	Expect(HelperFnDescriptionToken::RPAREN);

	auto attrs = ParseAttributes();
	for (const auto& attr : attrs) {
		descr->attributes.push_back(attr);
	}

	return descr;
}

std::string HelperFnDescriptionParser::ParseType()
{
	std::string type_spec = ParseTypeSpec();

	if (auto amp = Match(HelperFnDescriptionToken::AMPERSAND)) {
		return type_spec + "&";
	} else {
		return type_spec;
	}
}

std::string HelperFnDescriptionParser::ParseTypeSpec()
{
	std::string type_name = ParseTypeName();

	if (Match(HelperFnDescriptionToken::LBRACKET)) {
		auto size = Expect(HelperFnDescriptionToken::NUMBER)->GetValue();
		Expect(HelperFnDescriptionToken::RBRACKET);

		return type_name + "[" + size + "]";
	}

	return type_name;
}

std::string HelperFnDescriptionParser::ParseTypeName()
{
	return Expect(HelperFnDescriptionToken::IDENTIFIER)->GetValue();
}

HelperFnDescriptionParser::ParamsType HelperFnDescriptionParser::ParseParams()
{
	ParamsType params;

	if (Is(HelperFnDescriptionToken::RPAREN)) return params;

	ParamType param = ParseParam();
	params.push_back(param);

	while (Match(HelperFnDescriptionToken::COMMA)) {
		params.push_back(ParseParam());
	}

	return params;
}

HelperFnDescriptionParser::ParamType HelperFnDescriptionParser::ParseParam()
{
	std::string type = ParseType();
	std::string name = Expect(HelperFnDescriptionToken::IDENTIFIER)->GetValue();

	return ParamType(type, name);
}

std::vector<std::string> HelperFnDescriptionParser::ParseAttributes()
{
	std::vector<std::string> attrs;
	while (Is(HelperFnDescriptionToken::IDENTIFIER)) {
		attrs.push_back(Expect(HelperFnDescriptionToken::IDENTIFIER)->GetValue());
	}

	return attrs;
}
