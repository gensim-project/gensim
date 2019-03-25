/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/AsmDescriptionParser.h"

#include "flexbison_archcasm_ast.h"
#include "flexbison_archcasm.h"
#include "archCAsm.tabs.h"

using namespace gensim;
using namespace gensim::isa;

AsmDescriptionParser::AsmDescriptionParser(DiagnosticContext &diag, std::string filename) : diag(diag), description(new AsmDescription()), filename(filename) {}


AsmDescription *AsmDescriptionParser::Get()
{
	return description;
}

bool AsmDescriptionParser::Parse(const ArchC::AstNode &ptree, const ISADescription &isa)
{
	bool success = true;

	// tree node is set_asm node
	auto instr_name = ptree[0][0].GetString();
	auto format_string = ptree[1].GetString();

	description->instruction_name = instr_name;
	description->format = util::Util::Trim_Quotes(format_string);

	std::stringstream istr;
	istr.str(description->format);

	// Only parsing here in order to detect syntax errors
	ArchCAsm::ArchCAsmScanner scanner(&istr);
	ArchCAsm::AstNode astnode (ArchCAsmNodeType::ROOT);
	ArchCAsm::ArchCAsmParser parser(scanner, &astnode);

	if(parser.parse()) {
		diag.Error("Error while parsing asm statement", DiagNode("", ptree.GetLocation()));
		return false;
	}

	auto disasm_fields = isa.Get_Disasm_Fields();

	auto &operandListNode = ptree[2];
	for (auto operatorPtr : operandListNode) {
		auto &opNode = *operatorPtr;

		switch (opNode.GetType()) {
			case ArchCNodeType::Expression: {
				// constraint

				auto constrained_expression = util::expression::Parse(opNode);
				description->constraints.push_back(constrained_expression);
				break;
			}
			case ArchCNodeType::Identifier: {
				// argument
				std::string name = opNode[0].GetString();

				if (disasm_fields.find(name) == disasm_fields.end()) {
					diag.Error("Could not find a field named " + name, DiagNode(filename, opNode.GetLocation()));
					success = false;
				}

				description->args.push_back(util::expression::CreateID(name));
				break;
			}
		}
	}

	return success;
}
