/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/AsmDescriptionParser.h"

#include <antlr3.h>
#include <antlr-ver.h>
#include <archcasm/archcasmParser.h>
#include <archcasm/archcasmLexer.h>

#include <archc/archcLexer.h>

using namespace gensim;
using namespace gensim::isa;

AsmDescriptionParser::AsmDescriptionParser(DiagnosticContext &diag, std::string filename) : diag(diag), description(new AsmDescription()), filename(filename) {}


AsmDescription *AsmDescriptionParser::Get()
{
	return description;
}

bool AsmDescriptionParser::Parse(void *ptree, const ISADescription &isa)
{
	pANTLR3_BASE_TREE tree = (pANTLR3_BASE_TREE)ptree;
	bool success = true;

	// tree node is set_asm node
	pANTLR3_BASE_TREE instr_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	pANTLR3_BASE_TREE format_node = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	description->instruction_name = (char *)instr_node->getText(instr_node)->chars;
	description->format = util::Util::Trim_Quotes((char *)format_node->getText(format_node)->chars);

	pANTLR3_INPUT_STREAM pts = antlr3NewAsciiStringCopyStream((uint8_t *)description->format.c_str(), description->format.length(), (uint8_t *)"stream");
	parchcasmLexer lexer = archcasmLexerNew(pts);
	pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));

	parchcasmParser parser = archcasmParserNew(tstream);

	if (lexer->pLexer->rec->getNumberOfSyntaxErrors(lexer->pLexer->rec) > 0 || parser->pParser->rec->getNumberOfSyntaxErrors(parser->pParser->rec) > 0) {
		fprintf(stderr, "Errors detected while parsing asm statement (line %u)\n", tree->getLine(tree));
		return false;
	}

	archcasmParser_asmStmt_return asm_stmt = parser->asmStmt(parser);

	description->parsed_format = asm_stmt.tree;

	parser->free(parser);
	tstream->free(tstream);
	lexer->free(lexer);
	pts->free(pts);

	auto disasm_fields = isa.Get_Disasm_Fields();

	for (uint32_t i = 2; i < tree->getChildCount(tree); i++) {
		pANTLR3_BASE_TREE node = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
		switch (node->getType(node)) {
			case CONSTRAINT: {
				// constraint
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)node->getChild(node, 0);
				pANTLR3_BASE_TREE exprNode = (pANTLR3_BASE_TREE)node->getChild(node, 1);

				std::string fieldname = (char *)nameNode->getText(nameNode)->chars;

				if (disasm_fields.find(fieldname) == disasm_fields.end()) {
					diag.Error("Could not find a field named " + fieldname, DiagNode(filename, nameNode));
					success = false;
				}

				description->constraints.insert(std::pair<std::string, const util::expression *>((char *)nameNode->getText(nameNode)->chars, util::expression::Parse(exprNode)));
				break;
			}
			case ARG: {
				// argument
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)node->getChild(node, 0);

				std::string name = (char *)nameNode->getText(nameNode)->chars;

				if (disasm_fields.find(name) == disasm_fields.end()) {
					diag.Error("Could not find a field named " + name, DiagNode(filename, nameNode));
					success = false;
				}

				description->args.push_back(util::expression::Parse(nameNode));
				break;
			}
		}
	}

	return success;
}
