/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/ISADescriptionParser.h"
#include "isa/InstructionDescriptionParser.h"
#include "isa/AsmDescriptionParser.h"
#include "isa/AsmMapDescriptionParser.h"

#include "clean_defines.h"
#include "define.h"

#include "flexbison_archc_ast.h"
#include "flexbison_archc.h"

#include "flexbison_archcbehaviours_ast.h"
#include "flexbison_archcbehaviours.h"

#include <cstring>
#include <fstream>

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

	std::ifstream input_file (filename);
	if(!input_file.good()) {
		diag.Error("Could not open file " + filename, filename);
		return false;
	}

	ArchC::ArchCScanner scanner (&input_file);

	ArchC::AstNode astnode(ArchCNodeType::ROOT);
	ArchC::ArchCParser parser (scanner, &astnode);

	if(parser.parse()) {
		return false;
	}

	bool valid = load_from_node(astnode[0], filename);

	return valid;
}

ISADescription *ISADescriptionParser::Get()
{
	return isa;
}

bool ISADescriptionParser::parse_struct(ArchC::AstNode &node)
{
	bool success = true;

	StructDescription sd(node[0][0].GetString());

	auto &listNode = node[1];
	for(auto &entryPtr : listNode) {
		auto &entry = *entryPtr;

		std::string entry_name = entry[1][0].GetString();
		std::string entry_type = entry[0][0].GetString();

		sd.AddMember(entry_name, entry_type);
	}

	isa->UserStructTypes.push_back(sd);

	return success;
}


bool ISADescriptionParser::load_from_node(ArchC::AstNode &node, std::string filename)
{
	bool success = true;

	auto &nameNode = node[0];
	isa->ISAName = nameNode[0].GetString();

	auto &listNode = node[1];

	for (auto childPtr : listNode) {
		auto &child = *childPtr;

		switch (child.GetType()) {
			case ArchCNodeType::FetchSize: {
				isa->SetFetchLength(child[0].GetInt());
				break;
			}

			case ArchCNodeType::Include: {
				std::string filename = child[0].GetString();
				filename = filename.substr(1, filename.size()-2);
				success &= ParseFile(filename);
				break;
			}

			case ArchCNodeType::Format: {
				// trim quotes from format string
				std::string format_str = child[1].GetString();
				format_str = format_str.substr(1, format_str.length() - 2);

				std::string name_str = child[0][0].GetString();

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

				auto &attrListNode = child[2];
				for(auto attrPtr : attrListNode) {
					auto &attr = *attrPtr;
					fmt->AddAttribute(attr[0].GetString());

					if(attr[0].GetString() == "notpred") {
						fmt->SetCanBePredicated(false);
					}
				}

				if(isa->HasFormat(fmt->GetName())) {
					diag.Error("Instruction format " + name_str + " already defined", DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					isa->AddFormat(fmt);

					if ((fmt->GetLength() % isa->GetFetchLength()) != 0) {
						diag.Error("Instruction format " + name_str + " does not conform to ISA instruction fetch length", DiagNode(filename,  child.GetLocation()));
						success = false;
						continue;
					}
				}

				break;
			}
			case ArchCNodeType::AsmMap: {
				AsmMapDescription map = AsmMapDescriptionParser::Parse(child);
				isa->AddMapping(map);
				break;
			}
			case ArchCNodeType::Instruction: {
				auto &formatNode = child[0][0];
				std::string formatName = formatNode.GetString();;

				if (isa->Formats.find(formatName) == isa->Formats.end()) {
					diag.Error("Instruction specified with unknown format", DiagNode(filename, formatNode.GetLocation()));
					success = false;
					continue;
				}

				InstructionFormatDescription *format = (isa->Formats.at(formatName));

				auto &listNode = child[1];
				for(auto instrPtr : listNode) {
					auto &instr = *instrPtr;
					std::string instrName = instr[0].GetString();

					isa::ISADescription::InstructionDescriptionMap::const_iterator ins = isa->Instructions.find(instrName);
					if (ins != isa->Instructions.end()) {
						diag.Error("Duplicate instruction specified: " + instrName, DiagNode(filename, formatNode.GetLocation()));
						success = false;
						continue;
					}

					isa->AddInstruction(new InstructionDescription(instrName, *isa, format));
				}

				break;
			}
			case ArchCNodeType::Field: {
				FieldDescriptionParser parser (diag);
				if(parser.Parse(child)) {
					isa->UserFields.push_back(*parser.Get());
				}

				break;
			}
			case ArchCNodeType::IsaCtor: {

				success &= load_isa_from_node(child, filename);

				break;
			}
			case ArchCNodeType::BehavioursList: {
				auto &listNode = child[0];
				for (auto &entryPtr : listNode) {
					auto &behaviourNode = *entryPtr;
					std::string behaviour_name = behaviourNode[0].GetString();

					if(isa->ExecuteActionDeclared(behaviour_name)) {
						diag.Error("Behaviour action " + behaviour_name + " redefined", DiagNode(filename, behaviourNode.GetLocation()));
						success = false;
					} else {
						isa->DeclareExecuteAction(behaviour_name);
					}
				}
				break;
			}

			case ArchCNodeType::AcPredicated: {
				std::string text_value = child[0].GetString();
				isa->SetDefaultPredicated(text_value == "yes");
				break;
			}

			case ArchCNodeType::Struct: {
				success &= parse_struct(child);
				break;
			}

			default:
				UNEXPECTED;
		}
	}
	return success;
}


bool ISADescriptionParser::load_isa_from_node(ArchC::AstNode &node, std::string filename)
{
	using util::Util;

	bool success = true;

	auto &listNode = node[1];
	for (auto entryptr : listNode) {
		auto &child = *entryptr;

		switch (child.GetType()) {
			case ArchCNodeType::BehavioursFile: {
//				if (Util::Verbose_Level) printf("Loading behaviours from file %s\n", nameNode->getText(nameNode)->chars);
				std::string str = child[0].GetString();
				str = str.substr(1, str.length() - 2);
				isa->BehaviourFiles.push_back(str);
				break;
			}

			case ArchCNodeType::DecodesFile: {
				std::string str = child[0].GetString();
				str = str.substr(1, str.length() - 2);
//				if (Util::Verbose_Level) printf("Loading behaviours from file %s\n", str.c_str());
				isa->DecodeFiles.push_back(str);
				break;
			}

			case ArchCNodeType::ExecutesFile: {
				std::string str = child[0].GetString();
				str = str.substr(1, str.length() - 2);
				isa->ExecuteFiles.push_back(str);
				break;
			}

			case ArchCNodeType::Decoder: {
				std::string instrName = child[0][0].GetString();

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add decode constraints to unknown instruction", DiagNode(filename, child.GetLocation()));
					success = false;
					continue;
				} else {
					isa->instructions_.at(instrName)->bitStringsCalculated = false;
					if(!InstructionDescriptionParser::load_constraints_from_node(child[1], isa->instructions_.at(instrName)->Decode_Constraints))
						success = false;
				}
				break;
			}
			case ArchCNodeType::EndOfBlock: {
				std::string instrName = child[0][0].GetString();
//				if (Util::Verbose_Level) printf("Loading decode info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add end of block constraint to unknown instruction " + instrName, DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					isa->instructions_.at(instrName)->bitStringsCalculated = false;
					if(!InstructionDescriptionParser::load_constraints_from_node(child[1], isa->instructions_.at(instrName)->EOB_Contraints))
						success = false;
				}
				break;
			}
			case ArchCNodeType::UsesPc: {
				std::string instrName = child[0][0].GetString();
//				if (Util::Verbose_Level) printf("Loading decode info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add Uses-PC constraint to unknown instruction", DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					isa->instructions_.at(instrName)->bitStringsCalculated = false;
					if(!InstructionDescriptionParser::load_constraints_from_node(child[1], isa->instructions_.at(instrName)->Uses_PC_Constraints))
						success = false;
				}
				break;
			}
			case ArchCNodeType::JumpFixed:
			case ArchCNodeType::JumpFixedPredicated: {
				std::string instrName = child[0][0].GetString();
//				if (Util::Verbose_Level) printf("Loading jump info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add jump info to unknown instruction", DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					auto &fieldNode = child[1];
					InstructionDescription *instr = isa->instructions_.at(instrName);
					instr->FixedJumpField = fieldNode[0].GetString();

					if (child.GetType() == ArchCNodeType::JumpFixed) {
						instr->FixedJump = true;
						instr->FixedJumpPred = false;
					} else {
						instr->FixedJump = false;
						instr->FixedJumpPred = true;
					}

					auto &typeNode = child[2];
					GASSERT(typeNode.GetType() == ArchCNodeType::STRING);
					if (typeNode.GetString() == "Relative")
						instr->FixedJumpType = 1;
					else
						instr->FixedJumpType = 0;

					if (child.GetChildren().size() > 3) {
						auto &offsetNode = child[3];
						instr->FixedJumpOffset = offsetNode.GetInt();
					}
				}
				break;
			}
			case ArchCNodeType::JumpVariable: {
				std::string instrName = child[0][0].GetString();
//				if (Util::Verbose_Level) printf("Loading jump info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add jump info to unknown instruction", DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					InstructionDescription *instr = isa->instructions_.at(instrName);
					instr->VariableJump = true;
				}
				break;
			}
			case ArchCNodeType::BlockCond: {
				std::string instrName = child[0][0].GetString();

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add conditional block instruction info to unknown instruction", DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					InstructionDescription *instr = isa->instructions_.at(instrName);
					instr->blockCond = true;
				}

				break;
			}
			case ArchCNodeType::Assembler: {
				std::string instrName = child[0][0].GetString();
//				if (Util::Verbose_Level) printf("Loading disasm info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add disasm info to unknown instruction", DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					AsmDescriptionParser asm_parser (diag, filename);
					if(!asm_parser.Parse(child, *isa)) break;
					isa->instructions_[instrName]->Disasms.push_back(asm_parser.Get());
				}
				break;
			}
			case ArchCNodeType::Behaviour: {
				std::string instrName = child[0][0].GetString();
				std::string behaviourName = child[1][0].GetString();

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempted to add behaviour to unknown instruction " + instrName, DiagNode(filename, child.GetLocation()));
					success = false;
				} else {
					if(!isa->ExecuteActionDeclared(behaviourName)) {
						diag.Error("Attempted to add unknown behaviour " + behaviourName + " to instruction", DiagNode(filename, child.GetLocation()));
						success = false;
					} else {
						isa->SetBehaviour(instrName, behaviourName);
					}
				}

				break;
			}

			default: {
				UNEXPECTED;
			}
		}
	}

	isa->CleanupBehaviours();

//	printf("Loading behaviours...\n");
	success &= load_behaviours();

	return success;
}

std::string format_code(ArchCBehaviours::AstNode &node)
{
	switch(node.GetType()) {
		case ArchCBehavioursNodeType::List: {
			std::string outcode;
			for(auto child : node) {
				outcode += format_code(*child) + " ";
			}
			return outcode;
		}
		case ArchCBehavioursNodeType::STRING: {
			return node.GetString();
		}
		default: {
			UNEXPECTED;
		}
	}
}

bool ISADescriptionParser::load_behaviour_file(std::string filename)
{
	bool success = true;

	std::ifstream filestream(filename.c_str());
	if (!filestream) {
		std::cerr << "Could not find behaviour file " << filename << std::endl;
		return false;
	}

	if(loaded_files.count(filename)) return true;
	loaded_files.insert(filename);

	printf("Parsing behaviour file %s\n", filename.c_str());

	ArchCBehaviours::ArchCBehavioursScanner scanner (&filestream);
	ArchCBehaviours::AstNode root_node(ArchCBehavioursNodeType::ROOT);
	ArchCBehaviours::ArchCBehavioursParser parser(scanner, &root_node);

	if(parser.parse() != 0) {
		return false;
	}

	for (auto childPtr : root_node[0]) {
		auto &child = *childPtr;

		switch (child.GetType()) {
			case ArchCBehavioursNodeType::DecodeBehaviour: {
				std::string format = child[0].GetString();
				std::string code = format_code(child[1]);

				if (isa->Formats.find(format) != isa->Formats.end())
					isa->formats_[format]->DecodeBehaviourCode = code;
				else {
//					std::cerr << "Attempted to assign a behaviour string to a non-existant decode type " << format << "(Line " << child->getLine(child) << ")" << std::endl;
					success = false;
				}
				break;
			}
			case ArchCBehavioursNodeType::GenericBehaviour: {
				std::string name = child[0].GetString();
				std::string code = format_code(child[2]);

				isa->AddBehaviourAction(name, code);
				break;
			}

			default : {
				UNEXPECTED;
			}
		}
	}

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
