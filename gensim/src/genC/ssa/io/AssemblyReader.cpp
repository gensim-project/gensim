/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <cstring>
#include <fstream>
#include <sstream>
#include <ios>
#include <vector>

#include "flexbison_harness.h"
#include "flexbison_gencssa_ast.h"
#include "flexbison_gencssa.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "define.h"

using namespace gensim::genc::ssa::io;

bool AssemblyReader::Parse(const std::string& filename, gensim::DiagnosticContext& diag, AssemblyFileContext*& target) const
{
	std::ifstream file (filename, std::ios::in | std::ios::binary | std::ios::ate);

	GenCSSA::GenCSSAScanner scanner(&file);
	GenCSSA::AstNode root (GenCSSANodeType::ROOT);
	GenCSSA::GenCSSAParser parser(scanner, &root);

	if(parser.parse() != 0) {
		return false;
	}

	target = new AssemblyFileContext(root);
	return true;
}

bool AssemblyReader::ParseText(const std::string& text, gensim::DiagnosticContext& diag, AssemblyFileContext*& target) const
{
	std::stringstream ss;
	ss.str(text);

	GenCSSA::GenCSSAScanner scanner(&ss);
	GenCSSA::AstNode root (GenCSSANodeType::ROOT);
	GenCSSA::GenCSSAParser parser(scanner, &root);

	if(parser.parse() != 0) {
		return false;
	}

	target = new AssemblyFileContext(root);
	return true;
}
