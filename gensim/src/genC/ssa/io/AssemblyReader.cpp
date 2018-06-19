/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <cstring>
#include <fstream>
#include <ios>
#include <vector>

#include <antlr3.h>
#include "antlr-ver.h"

#include "ssa_asm/ssa_asmLexer.h"
#include "ssa_asm/ssa_asmParser.h"
#include "genC/ssa/io/AssemblyReader.h"

using namespace gensim::genc::ssa::io;

static pANTLR3_INPUT_STREAM GetFile(const std::string &str)
{
	FILE *f = fopen(str.c_str(), "r");
	if(f == nullptr) {
		return nullptr;
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *data = (char*)malloc(size);
	fread(data, 1, size, f);

	// check data
	for(size_t i = 0; i < size; ++i) {
		char c= data[i];
		if(!isascii(c)) {
			return nullptr;
		}
	}

	return antlr3StringStreamNew((pANTLR3_UINT8)data, ANTLR3_ENC_8BIT, size, (pANTLR3_UINT8)strdup(str.c_str()));
}

bool AssemblyReader::Parse(const std::string& filename, gensim::DiagnosticContext& diag, AssemblyFileContext*& target) const
{
	std::ifstream file (filename, std::ios::in | std::ios::binary | std::ios::ate);

	std::ifstream::pos_type filesize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> data(filesize);
	file.read(data.data(), filesize);

	std::string text (data.data(), filesize);
	for(auto i : text) {
		if(!isspace(i) && !isgraph(i)) {
			diag.AddEntry(DiagnosticClass::Error, "Found a non-printable character ("+std::to_string((int)i)+")");
			return false;
		}
	}

	return ParseText(text, diag, target);
}

bool AssemblyReader::ParseText(const std::string& text, gensim::DiagnosticContext& diag, AssemblyFileContext*& target) const
{
	auto stringfactory = antlr3StringFactoryNew(ANTLR3_ENC_8BIT);
	auto string = stringfactory->newStr(stringfactory, (unsigned char *)text.c_str());
	pANTLR3_INPUT_STREAM pts = antlr3StringStreamNew(string->chars, ANTLR3_ENC_8BIT, text.size(), (unsigned char*)"");

	pssa_asmLexer lexer = ssa_asmLexerNew(pts);
	pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	pssa_asmParser parser = ssa_asmParserNew(tstream);

	ssa_asmParser_asm_file_return context_tree = parser->asm_file(parser);

	if (parser->pParser->rec->getNumberOfSyntaxErrors(parser->pParser->rec) > 0 || lexer->pLexer->rec->getNumberOfSyntaxErrors(lexer->pLexer->rec)) {
		return false;
	}

	target = new AssemblyFileContext(context_tree.tree);
	return true;
}
