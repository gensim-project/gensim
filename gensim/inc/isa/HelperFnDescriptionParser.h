/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <istream>
#include <vector>
#include <utility>

namespace gensim
{
	class DiagnosticContext;

	namespace isa
	{
		class HelperFnDescription;

		class HelperFnDescriptionToken
		{
		public:
			enum TokenKind {
				PRIVATE,
				PUBLIC,
				INTERNAL,
				STRUCT,

				HELPER,

				IDENTIFIER,
				NUMBER,

				LBRACKET,
				RBRACKET,
				LPAREN,
				RPAREN,
				COMMA,
				AMPERSAND,

				UNKNOWN
			};

			HelperFnDescriptionToken(TokenKind kind, const std::string& value) : kind_(kind), value_(value) { }

			TokenKind GetKind() const
			{
				return kind_;
			}
			const std::string& GetValue() const
			{
				return value_;
			}

		private:
			TokenKind kind_;
			const std::string value_;
		};

		class HelperFnDescriptionLexer
		{
		public:
			HelperFnDescriptionLexer(std::istream& stream);

			const HelperFnDescriptionToken *Peek() const
			{
				return current_;
			}
			const HelperFnDescriptionToken *Pop();

		private:
			char PeekChar() const
			{
				return current_char_;
			}
			char PopChar();

			const HelperFnDescriptionToken *ReadIdentifier();
			const HelperFnDescriptionToken *ReadNumber();

			void SkipWhitespace();

			std::istream&  stream_;
			const HelperFnDescriptionToken *current_;

			char current_char_;
		};

		class HelperFnDescriptionParser
		{
		public:
			HelperFnDescriptionParser(HelperFnDescriptionLexer& lexer) : lexer_(lexer) { }

			HelperFnDescription *Parse(const std::string& body);

		private:
			const HelperFnDescriptionToken *Expect(HelperFnDescriptionToken::TokenKind kind);
			const HelperFnDescriptionToken *Match(HelperFnDescriptionToken::TokenKind kind);
			const HelperFnDescriptionToken *Is(HelperFnDescriptionToken::TokenKind kind);

			HelperFnDescription *ParsePrototype(const std::string& body);
			std::string ParseType();
			std::string ParseTypeSpec();
			std::string ParseTypeQual();
			std::string ParseTypeName();

			typedef std::pair<std::string, std::string> ParamType;
			typedef std::vector<ParamType> ParamsType;
			ParamsType ParseParams();
			ParamType ParseParam();

			std::vector<std::string> ParseAttributes();

			HelperFnDescriptionLexer& lexer_;
		};
	}
}
