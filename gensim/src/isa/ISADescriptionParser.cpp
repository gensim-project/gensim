/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/ISADescriptionParser.h"
#include "isa/InstructionDescriptionParser.h"
#include "isa/AsmDescriptionParser.h"
#include "isa/AsmMapDescriptionParser.h"

#include <archcasm/archcasmParser.h>
#include <archcasm/archcasmLexer.h>
#include <archCBehaviour/archCBehaviourLexer.h>
#include <archCBehaviour/archCBehaviourParser.h>

#include "clean_defines.h"
#include "define.h"
#include <archc/archcParser.h>
#include "clean_defines.h"
#include <archc/archcLexer.h>

#include <cstring>
#include <fstream>

#include <antlr3.h>
#include <antlr-ver.h>

using namespace gensim;
using namespace gensim::isa;

ISADescriptionParser::ISADescriptionParser(DiagnosticContext &diag, uint8_t isa_size) : diag(diag), isa(new ISADescription(isa_size))
{

}

bool ISADescriptionParser::ParseFile(std::string filename)
{
	if(parsed_files.count(filename)) {
		diag.Error("Detected recursive inclusion of " + filename, filename);
		return false;
	}

	parsed_files.insert(filename);

	std::ifstream test (filename);
	if(!test.good()) {
		diag.Error("Could not open file " + filename, filename);
		return false;
	}

	pANTLR3_UINT8 fname_str = (pANTLR3_UINT8)filename.c_str();
	pANTLR3_INPUT_STREAM pts = antlr3AsciiFileStreamNew(fname_str);
	parchcLexer lexer = archcLexerNew(pts);
	pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	parchcParser parser = archcParserNew(tstream);

	archcParser_arch_isa_return arch = parser->arch_isa(parser);

	if (parser->pParser->rec->getNumberOfSyntaxErrors(parser->pParser->rec) > 0 || lexer->pLexer->rec->getNumberOfSyntaxErrors(lexer->pLexer->rec)) {
		diag.Error("Syntax errors detected in ISA Description", filename);
		return false;
	}

	pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(arch.tree, ANTLR3_SIZE_HINT);

	bool valid = load_from_node(nodes->root, filename);

//	 FIXME: free these safely
	nodes->free(nodes);
	parser->free(parser);
	tstream->free(tstream);
	lexer->free(lexer);
	pts->free(pts);

	return valid;
}

ISADescription *ISADescriptionParser::Get()
{
	return isa;
}

bool ISADescriptionParser::parse_struct(pANTLR3_BASE_TREE node)
{
	bool success = true;
	auto struct_name_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);

	StructDescription sd(std::string((char*)struct_name_node->getText(struct_name_node)->chars));

	for(unsigned i = 1; i < node->getChildCount(node); i += 2) {
		auto entry_name_node = (pANTLR3_BASE_TREE)node->getChild(node, i+1);
		auto entry_type_node = (pANTLR3_BASE_TREE)node->getChild(node, i);

		std::string entry_name = std::string((char*)entry_name_node->getText(entry_name_node)->chars);
		std::string entry_type = std::string((char*)entry_type_node->getText(entry_type_node)->chars);

		sd.AddMember(entry_name, entry_type);
	}

	isa->UserStructTypes.push_back(sd);

	return success;
}


bool ISADescriptionParser::load_from_node(pANTLR3_BASE_TREE node, std::string filename)
{
	bool success = true;

	for (uint32_t i = 0; i < node->getChildCount(node); i++) {
		pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)node->getChild(node, i);
		switch (child->getType(child)) {
			case AC_FETCHSIZE: {
				pANTLR3_BASE_TREE sizeNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				isa->SetFetchLength(atoi((char *)(sizeNode->getText(sizeNode)->chars)));
				break;
			}

			case AC_INCLUDE: {
				assert(child->getChildCount(child) == 1);
				pANTLR3_BASE_TREE filenameNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string filename = std::string((char*)filenameNode->getText(filenameNode)->chars);
				filename = filename.substr(1, filename.size()-2);
				success &= ParseFile(filename);
				break;
			}

			case AC_FORMAT: {
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				pANTLR3_BASE_TREE formatNode = (pANTLR3_BASE_TREE)child->getChild(child, 1);

				// trim quotes from format string
				std::string format_str = (char *)formatNode->getText(formatNode)->chars;
				format_str = format_str.substr(1, format_str.length() - 2);

				std::string name_str = (char *)nameNode->getText(nameNode)->chars;

				InstructionFormatDescriptionParser parser (diag, *isa);
				InstructionFormatDescription *fmt;
				if(!parser.Parse(name_str, format_str, fmt)) {
					success = false;
					break;
				}

				if(!fmt) {
					success = false;
					break;
				}

				for(int i = 2; i < child->getChildCount(child); ++i) {
					pANTLR3_BASE_TREE attrNode = (pANTLR3_BASE_TREE)child->getChild(child, i);
					fmt->AddAttribute(std::string((char*)attrNode->getText(attrNode)->chars));

					if(!strcmp((char*)attrNode->getText(attrNode)->chars, "notpred")) {
						fmt->SetCanBePredicated(false);
					}
				}

				if(isa->HasFormat(fmt->GetName())) {
					diag.Error("Instruction format " + name_str + " already defined", DiagNode(filename, formatNode));
					success = false;
				} else {
					isa->AddFormat(fmt);

					if ((fmt->GetLength() % isa->GetFetchLength()) != 0) {
						diag.Error("Instruction format " + name_str + " does not conform to ISA instruction fetch length", DiagNode(filename, formatNode));
						success = false;
						continue;
					}
				}

				break;
			}
			case AC_ASM_MAP: {
				AsmMapDescription map = AsmMapDescriptionParser::Parse(child);
				isa->AddMapping(map);
				break;
			}
			case AC_INSTR: {
				pANTLR3_BASE_TREE formatNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string formatName = (char *)(formatNode->getText(formatNode)->chars);

				if (isa->Formats.find(formatName) == isa->Formats.end()) {
					diag.Error("Instruction specified with unknown format", DiagNode(filename, formatNode));
					success = false;
					continue;
				}

				InstructionFormatDescription *format = (isa->Formats.at(formatName));

				for (uint32_t j = 1; j < child->getChildCount(child); j++) {
					pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, j);
					std::string instrName = (char *)(instrNode->getText(instrNode)->chars);

					isa::ISADescription::InstructionDescriptionMap::const_iterator ins = isa->Instructions.find(instrName);
					if (ins != isa->Instructions.end()) {
						diag.Error("Duplicate instruction specified: " + instrName, DiagNode(filename, formatNode));
						success = false;
						continue;
					}

					isa->AddInstruction(new InstructionDescription(instrName, *isa, format));
				}

				break;
			}
			case AC_FIELD: {
				FieldDescriptionParser parser (diag);
				if(parser.Parse(child)) {
					isa->UserFields.push_back(*parser.Get());
				}

				break;
			}
			case ISA_CTOR: {

				success &= load_isa_from_node(child, filename);

				break;
			}
			case AC_BEHAVIOUR: {
				for (int i = 0; i < child->getChildCount(child); ++i) {
					pANTLR3_BASE_TREE behaviour_node = (pANTLR3_BASE_TREE)child->getChild(child, i);
					std::string behaviour_name = (char *)behaviour_node->getText(behaviour_node)->chars;

					if(isa->ExecuteActionDeclared(behaviour_name)) {
						diag.Error("Behaviour action " + behaviour_name + " redefined", DiagNode(filename, behaviour_node));
						success = false;
					} else {
						isa->DeclareExecuteAction(behaviour_name);
					}
				}
				break;
			}
			case AC_ID: {
				isa->ISAName = std::string((const char *)child->getText(child)->chars);
				break;
			}
			case AC_PREDICATED: {
				pANTLR3_BASE_TREE value = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				isa->SetDefaultPredicated(strcmp((const char *)value->getText(value)->chars, "yes") == 0);
				break;
			}

			case AC_STRUCT: {
				success &= parse_struct(child);
				break;
			}

			default:
				diag.Error("Internal error: Unknown node type on line " + std::to_string(child->getLine(child)));
				success = false;
				break;
		}
	}
	return success;
}


bool ISADescriptionParser::load_isa_from_node(pANTLR3_BASE_TREE node, std::string filename)
{
	using util::Util;

	bool success = true;
	for (uint32_t i = 0; i < node->getChildCount(node); i++) {
		pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)node->getChild(node, i);
		switch (child->getType(child)) {
			case AC_MODIFIERS: {
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
//				printf("Loading modifiers from file %s\n", nameNode->getText(nameNode)->chars);
				std::string str = (char *)nameNode->getText(nameNode)->chars;
				str = str.substr(1, str.length() - 2);
				isa->SetProperty("Modifiers", str);
				break;
			}

			case AC_BEHAVIOURS: {
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
//				if (Util::Verbose_Level) printf("Loading behaviours from file %s\n", nameNode->getText(nameNode)->chars);
				std::string str = (char *)nameNode->getText(nameNode)->chars;
				str = str.substr(1, str.length() - 2);
				isa->BehaviourFiles.push_back(str);
				break;
			}

			case AC_DECODES: {
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
//				if (Util::Verbose_Level) printf("Loading behaviours from file %s\n", nameNode->getText(nameNode)->chars);
				std::string str = (char *)nameNode->getText(nameNode)->chars;
				str = str.substr(1, str.length() - 2);
//				if (Util::Verbose_Level) printf("Loading behaviours from file %s\n", str.c_str());
				isa->DecodeFiles.push_back(str);
				break;
			}

			case AC_EXECUTE: {
				pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
//				if (Util::Verbose_Level) printf("Loading behaviours from file %s\n", nameNode->getText(nameNode)->chars);
				std::string str = (char *)nameNode->getText(nameNode)->chars;
				str = str.substr(1, str.length() - 2);
				isa->ExecuteFiles.push_back(str);
				break;
			}

			case SET_DECODER: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading decode info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add decode constraints to unknown instruction", DiagNode(filename, instrNode));
					success = false;
					continue;
				} else {
					isa->instructions_.at(instrName)->bitStringsCalculated = false;
					if(!InstructionDescriptionParser::load_constraints_from_node(child, isa->instructions_.at(instrName)->Decode_Constraints))
						success = false;
				}
				break;
			}
			case SET_EOB: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading decode info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add end of block constraint to unknown instruction " + instrName, DiagNode(filename, instrNode));
					success = false;
				} else {
					isa->instructions_.at(instrName)->bitStringsCalculated = false;
					if(!InstructionDescriptionParser::load_constraints_from_node(child, isa->instructions_.at(instrName)->EOB_Contraints))
						success = false;
				}
				break;
			}
			case SET_USES_PC: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading decode info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add Uses-PC constraint to unknown instruction", DiagNode(filename, instrNode));
					success = false;
				} else {
					isa->instructions_.at(instrName)->bitStringsCalculated = false;
					if(!InstructionDescriptionParser::load_constraints_from_node(child, isa->instructions_.at(instrName)->Uses_PC_Constraints))
						success = false;
				}
				break;
			}
			case SET_JUMP_FIXED:
			case SET_JUMP_FIXED_PREDICATED: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading jump info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add jump info to unknown instruction", DiagNode(filename, instrNode));
					success = false;
				} else {
					pANTLR3_BASE_TREE fieldNode = (pANTLR3_BASE_TREE)child->getChild(child, 1);
					InstructionDescription *instr = isa->instructions_.at(instrName);
					instr->FixedJumpField = std::string((char *)fieldNode->getText(fieldNode)->chars);

					if (child->getType(child) == SET_JUMP_FIXED) {
						instr->FixedJump = true;
						instr->FixedJumpPred = false;
					} else {
						instr->FixedJump = false;
						instr->FixedJumpPred = true;
					}

					pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE)child->getChild(child, 2);
					if (typeNode->getType(typeNode) == RELATIVE)
						instr->FixedJumpType = 1;
					else
						instr->FixedJumpType = 0;

					if (child->getChildCount(child) > 3) {
						pANTLR3_BASE_TREE offsetNode = (pANTLR3_BASE_TREE)child->getChild(child, 3);
						instr->FixedJumpOffset = atoi((char *)(offsetNode->getText(offsetNode)->chars));
					}
				}
				break;
			}
			case SET_JUMP_VARIABLE: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading jump info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add jump info to unknown instruction", DiagNode(filename, instrNode));
					success = false;
				} else {
					InstructionDescription *instr = isa->instructions_.at(instrName);
					instr->VariableJump = true;
				}
				break;
			}
			case SET_BLOCK_COND: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add conditional block instruction info to unknown instruction", DiagNode(filename, instrNode));
					success = false;
				} else {
					InstructionDescription *instr = isa->instructions_.at(instrName);
					instr->blockCond = true;
				}

				break;
			}
			case SET_ASM: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading disasm info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add disasm info to unknown instruction", DiagNode(filename, instrNode));
					success = false;
				} else {
					AsmDescriptionParser asm_parser (diag, filename);
					if(!asm_parser.Parse(child, *isa)) break;
					isa->instructions_[instrName]->Disasms.push_back(asm_parser.Get());
				}
				break;
			}
			case SET_BEHAVIOUR: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				pANTLR3_BASE_TREE behaviourNode = (pANTLR3_BASE_TREE)child->getChild(child, 1);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
				std::string behaviourName = (char *)behaviourNode->getText(behaviourNode)->chars;

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempted to add behaviour to unknown instruction " + instrName, DiagNode(filename, child));
					success = false;
				} else {
					if(!isa->ExecuteActionDeclared(behaviourName)) {
						diag.Error("Attempted to add unknown behaviour " + behaviourName + " to instruction", DiagNode(filename, child));
						success = false;
					} else {
						isa->SetBehaviour(instrName, behaviourName);
					}
				}

				break;
			}
			case SET_HAS_LIMM: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				pANTLR3_BASE_TREE limmLengthNode = (pANTLR3_BASE_TREE)child->getChild(child, 1);

				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
				int limmLength = atoi((char *)limmLengthNode->getText(limmLengthNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
//					fprintf(stderr, "Attempting to set limm for unknown instruction %s (Line %u)\n", instrName.c_str(), instrNode->getLine(instrNode));
					success = false;
				}

				if (success) {
					InstructionDescription *inst = isa->instructions_[instrName];
					inst->LimmCount = limmLength;
				}

				break;
			}
		}
	}

	isa->CleanupBehaviours();

//	printf("Loading behaviours...\n");
	success &= load_behaviours();

	return success;
}

bool ISADescriptionParser::load_behaviour_file(std::string filename)
{
	bool success = true;

	std::ifstream _check_file(filename.c_str());
	if (!_check_file) {
		std::cerr << "Could not find behaviour file " << filename << std::endl;
		return false;
	}

	if(loaded_files.count(filename)) return true;
	loaded_files.insert(filename);

//	printf("Parsing behaviour file %s\n", filename.c_str());

	pANTLR3_UINT8 fname_str = (pANTLR3_UINT8)filename.c_str();
	pANTLR3_INPUT_STREAM pts = antlr3AsciiFileStreamNew(fname_str);
	parchCBehaviourLexer lexer = archCBehaviourLexerNew(pts);
	pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	parchCBehaviourParser parser = archCBehaviourParserNew(tstream);

	archCBehaviourParser_behaviour_file_return arch = parser->behaviour_file(parser);

	pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(arch.tree, ANTLR3_SIZE_HINT);

	pANTLR3_BASE_TREE behaviourTree = nodes->root;

	for (uint32_t behaviourIndex = 0; behaviourIndex < behaviourTree->getChildCount(behaviourTree); behaviourIndex++) {
		pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)behaviourTree->getChild(behaviourTree, behaviourIndex);
		switch (child->getType(child)) {
			case BE_DECODE: {
				pANTLR3_BASE_TREE nameChild = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string format = (char *)nameChild->getText(nameChild)->chars;

				pANTLR3_BASE_TREE codeChild = (pANTLR3_BASE_TREE)child->getChild(child, 1);
				std::string code = (char *)codeChild->getText(codeChild)->chars;

				if (isa->Formats.find(format) != isa->Formats.end())
					isa->formats_[format]->DecodeBehaviourCode = code;
				else {
					std::cerr << "Attempted to assign a behaviour string to a non-existant decode type " << format << "(Line " << child->getLine(child) << ")" << std::endl;
					success = false;
				}
				break;
			}
			case BE_EXECUTE: {
				pANTLR3_BASE_TREE nameChild = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string format = (char *)nameChild->getText(nameChild)->chars;

				pANTLR3_BASE_TREE codeChild = (pANTLR3_BASE_TREE)child->getChild(child, 1);
				std::string code = (char *)codeChild->getText(codeChild)->chars;

				// if (ExecuteActions.find(format) != ExecuteActions.end())
				if(!isa->ExecuteActionDeclared(format)) {
					diag.Error("Adding behaviour to unknown execute action " + format, DiagNode(filename, nameChild));
					success = false;
				} else {
					isa->AddExecuteAction(format, code);
				}
				// else
				//{
				// std::cerr << "Attempted to assign a behaviour string to an unknown behaviour " << format << std::endl;
				// success = false;
				//}
				break;
			}
			case BE_BEHAVIOUR: {
				pANTLR3_BASE_TREE nameChild = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string name = (char *)nameChild->getText(nameChild)->chars;

				pANTLR3_BASE_TREE codeChild = (pANTLR3_BASE_TREE)child->getChild(child, 1);
				std::string code = (char *)codeChild->getText(codeChild)->chars;

				isa->AddBehaviourAction(name, code);
				break;
			}

			case BE_HELPER: {
				pANTLR3_BASE_TREE prototypeChild = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				pANTLR3_BASE_TREE bodyChild = (pANTLR3_BASE_TREE)child->getChild(child, 1);

				std::string prototype;

				for (uint32_t i = 0; i < prototypeChild->getChildCount(prototypeChild); ++i) {
					pANTLR3_BASE_TREE node = (pANTLR3_BASE_TREE)prototypeChild->getChild(prototypeChild, i);
					prototype.append((char *)node->getText(node)->chars);
					prototype.append(" ");
				}

				std::string body = (char *)bodyChild->getText(bodyChild)->chars;

				if (util::Util::Verbose_Level) std::cout << "Loading helper function " << prototype << std::endl;

				isa->AddHelperFn(prototype, body);
				break;
			}
		}
	}

//	FIXME: free these safely
	nodes->free(nodes);
	parser->free(parser);
	tstream->free(tstream);
	lexer->free(lexer);
	pts->free(pts);

	return success;
}

bool ISADescriptionParser::load_behaviours()
{
	bool success = true;
	for (std::vector<std::string>::iterator ci = isa->BehaviourFiles.begin(); ci != isa->BehaviourFiles.end(); ++ci) success &= load_behaviour_file(*ci);
	for (std::vector<std::string>::iterator ci = isa->DecodeFiles.begin(); ci != isa->DecodeFiles.end(); ++ci) success &= load_behaviour_file(*ci);
//	for (std::vector<std::string>::iterator ci = isa->ExecuteFiles.begin(); ci != isa->ExecuteFiles.end(); ++ci) success &= load_behaviour_file(*ci);
	return success;
}
