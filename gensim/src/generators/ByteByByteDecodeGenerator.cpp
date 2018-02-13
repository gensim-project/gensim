/*
 * ByteByByteDecodeGenerator.cpp
 *
 *  Created on: Oct 10, 2012
 *      Author: harry
 */
#if 0
#include "ArchDescription.h"
#include "ISADescription.h"

#include "DecodeTree.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"

#include "generators/ByteByByteDecodeGenerator.h"

DEFINE_COMPONENT(gensim::generator::ByteByByteDecodeGenerator, decode_bbb)
COMPONENT_INHERITS(decode_bbb, base_decode);

namespace gensim
{
	namespace generator
	{
		bool ByteByByteDecodeGenerator::GenerateDecodeHeader(std::stringstream &header_str) const
		{

			bool success = true;
			const arch::ArchDescription &Architecture = Manager.GetArch();
			const InterpretiveExecutionEngineGenerator &interpret = (const InterpretiveExecutionEngineGenerator&) *Manager.GetComponentC(GenerationManager::FnInterpret);

			header_str << "/* Auto Generated Decoded Instruction Class for Arch " << Architecture.Name << " */\n";
			header_str << "#ifndef __ARCH_" << Architecture.Name << "_DECODE_INSTR_HEADER \n";
			header_str << "#define __ARCH_" << Architecture.Name << "_DECODE_INSTR_HEADER \n";

			header_str << "\n#include <define.h>\n\n";

			header_str << "#include <gensim/gensim_decode.h>\n";

			header_str << "namespace gensim {\n";
			header_str << "namespace " << Architecture.Name << "{\n";

			header_str << "class Processor;\n";

			//generate enum for instruction types
			header_str << "enum " << GetProperty("class") << "_Enum \n{\n";
			uint32_t longest_insn = 0;
			for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = Architecture.ISA->Instructions.begin(); i != Architecture.ISA->Instructions.end(); ++i) {
				header_str << "\tINST_" << i->second->Name << ",\n";
				if (i->second->Format->GetLength() > longest_insn)
					longest_insn = i->second->Format->GetLength();
			}
			header_str << "};\n\n"; //enum

			//generate class for decoded instruction
			header_str << "class " << GetProperty("class") << " : public gensim::BaseDecode\n{\n";

			header_str << "public:\n";

			header_str << "//Built in fields\n";
			//header_str << "\t" << GetProperty("class") << "_Enum\tInstr_Code;\n";

			header_str << "uint" << Architecture.ISA->GetFetchLength() << " insn_ir[" << (longest_insn / Architecture.ISA->GetFetchLength()) << "];";

			header_str << "//User-defined fields\n";
			for (std::list<isa::FieldDescription>::iterator i = Architecture.ISA->UserFields.begin(); i != Architecture.ISA->UserFields.end(); ++i) {
				header_str << "\t " << i->field_type << " " << i->field_name << ";\n";
			}

			header_str << "\n//Instruction Fields\n";

			std::map<std::string, uint32_t> fields = Architecture.ISA->Get_Decode_Fields();
			for (std::map<std::string, uint32_t>::iterator i = fields.begin(); i != fields.end(); ++i) {
				std::string type = util::Util::TypeString(i->second);
				header_str << "\t" << type << "\t" << i->first << ";\n";
			}

			header_str << "\n";
			header_str << "void DecodeInstr(uint32_t, gensim::Processor &);\n\n";

			//generate type-specific decode functions if they exist

			for (std::map<std::string, isa::InstructionFormat*>::const_iterator i = Architecture.ISA->Formats.begin(); i != Architecture.ISA->Formats.end(); ++i) {
				if (i->second->DecodeBehaviourCode.length() > 0) {
					if (GetProperty("InlineHints") == "1")
						header_str << "inline ";
					header_str << "void " << "decode_behaviour_" << i->second->Name << "(Processor&";

					for (std::list<isa::InstructionFormatChunk>::const_iterator ifc = i->second->GetChunks().begin(); ifc != i->second->GetChunks().end(); ++ifc) {
						if (!ifc->generate_field && !ifc->is_constrained)
							header_str << ", " << util::Util::TypeString(ifc->length);
					}

					header_str << ");\n";
				}
			}

			for (std::map<std::string, isa::InstructionFormat*>::const_iterator i = Architecture.ISA->Formats.begin(); i != Architecture.ISA->Formats.end(); ++i) {
				if (GetProperty("InlineHints") == "1")
					header_str << "inline ";
				header_str << "void ";
				header_str << "Decode_Format_" << i->first << "(uint32_t, gensim::Processor &);\n";
			}

			if (interpret.GetProperty("StaticPredication") == "1")
				header_str << "bool is_predicated();";

			header_str << "};\n\n"; //class

			header_str << "} }\n\n"; //namespaces

			header_str << "#endif\n";
			return success;
		}

		bool ByteByByteDecodeGenerator::GenerateDecodeSource(std::stringstream &source_str) const
		{
			bool success = true;
			const arch::ArchDescription &Architecture = Manager.GetArch();
			const InterpretiveExecutionEngineGenerator &interpret = (const InterpretiveExecutionEngineGenerator&) *Manager.GetComponentC(GenerationManager::FnInterpret);

			source_str << "#include <stdio.h>\n\n";
			source_str << "#include \"decode.h\"\n\n";
			source_str << "#include <gensim/gensim_define.h>\n\n";
			source_str << "#include <util/Bitfield.h>\n";
			source_str << "#include \"processor.h\"\n";

			source_str << "#define INTERP\n";
			source_str << "#define PREFIX cpu.\n";
			source_str << "#include <gensim/gensim_processor_api.h>\n";
			source_str << "#undef INTERP\n";

			source_str << "namespace gensim { \n namespace " << Architecture.Name << "{ \n";

			if (interpret.GetProperty("StaticPredication") == "1") {
				source_str << "bool " << GetProperty("class") << "::is_predicated() { " << GetProperty("class") << "*curr_interp_insn = this;";
				source_str << Architecture.ISA->GetBehaviourAction("is_predicated");
				source_str << "}";
			}

			source_str << "void " << GetProperty("class") << "::DecodeInstr(uint32_t pc, gensim::Processor &cpu)\n{\n";

			std::map<std::string, std::list<const isa::InstructionDescription*> > ins_formats;
			for (isa::ISADescription::InstructionFormatMap::const_iterator i = Architecture.ISA->Formats.begin(); i != Architecture.ISA->Formats.end(); ++i) {
				ins_formats[i->second->Name] = std::list<const isa::InstructionDescription*>();
			}

			for (isa::ISADescription::InstructionDescriptionMap::const_iterator ci = Architecture.ISA->Instructions.begin(); ci != Architecture.ISA->Instructions.end(); ++ci) {
				ins_formats[ci->second->Format->Name].push_back(ci->second);
			}

			source_str << "uint32 instr, efa;";
			source_str << "cpu.fetch32(pc, instr, efa, false);";

			source_str << "Instr_Code = (" << GetProperty("class") << "_Enum)-1;\n";
			source_str << "ClearEndOfBlock();\n";
			source_str << "SetIR(instr);\n";
#ifdef ENABLE_LIMM_OPERATIONS
			source_str << "LimmPtr = 0;\n";
			source_str << "LimmBytes = 0;\n";
#endif
			//recursively emit decode statements for the tree
			int n = 0;
			success &= GenerateDecodeTree(*decode_tree, source_str, n);

			source_str << "}\n\n";
			//generate type-specific decode functions if they exist

			for (std::map<std::string, isa::InstructionFormat*>::const_iterator i = Architecture.ISA->Formats.begin(); i != Architecture.ISA->Formats.end(); ++i) {
				isa::InstructionFormat *ins = i->second;
				if (i->second->DecodeBehaviourCode.length() > 0) {
					source_str << "void " << GetProperty("class") << "::decode_behaviour_" << i->second->Name << "(Processor &cpu";

					for (std::list<isa::InstructionFormatChunk>::const_iterator j = ins->GetChunks().begin(); j != ins->GetChunks().end(); ++j) {
						if (!j->generate_field && !j->is_constrained)
							source_str << ", " << util::Util::TypeString(j->length) << " " << j->Name;
					}
					source_str << ")\n" << i->second->DecodeBehaviourCode << "\n\n";
				}
			}

			uint32_t longest_insn = 0;
			for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = Architecture.ISA->Instructions.begin(); i != Architecture.ISA->Instructions.end(); ++i) {
				if (i->second->Format->GetLength() > longest_insn)
					longest_insn = i->second->Format->GetLength();
			}

			//generate decode functions for each instruction format
			for (isa::ISADescription::InstructionFormatMap::const_iterator i = Architecture.ISA->Formats.begin(); i != Architecture.ISA->Formats.end(); ++i) {
				source_str << "void " << GetProperty("class") << "::Decode_Format_" << i->first << "(uint32_t instr, gensim::Processor &cpu)\n{";

				std::stringstream param_str;

				//build a struct representing the exact instruction encoding using bitfields
				source_str << "archsim::util::BitArray<" << util::Util::TypeString(Architecture.ISA->GetFetchLength()) << ", " << longest_insn / Architecture.ISA->GetFetchLength() << "> __bits (insn_ir);";

				unsigned pos = 0;
				for (std::list<isa::InstructionFormatChunk>::const_iterator chunk_i = i->second->GetChunks().begin(); chunk_i != i->second->GetChunks().end(); ++chunk_i) {
					if (chunk_i->is_constrained) {
						pos += chunk_i->length;
						continue;
					}

					if (chunk_i->generate_field)
						source_str << chunk_i->Name << " = __bits.GetBits<" << util::Util::TypeString(chunk_i->length) << ", " << pos << ", " << (uint16_t) chunk_i->length << ">();";
					else {
						source_str << util::Util::TypeString(chunk_i->length) << " " << chunk_i->Name << " = __bits.GetBits<" << util::Util::TypeString(chunk_i->length) << ", " << pos << ", " << (uint16_t) chunk_i->length << ">();";
						param_str << ", " << chunk_i->Name;
					}

					pos += chunk_i->length;
				}

				if (i->second->DecodeBehaviourCode.length() > 0) {
					source_str << "\tdecode_behaviour_" << i->second->Name << "(static_cast<Processor&>(cpu)" << param_str.str() << ");\n";
				}

				//if we're interpreting with static predication, set the predication here

				if (interpret.GetProperty("StaticPredication") == "1") {
					source_str << "IsPredicated = is_predicated();\n";
				}

				//create code for end-of-block matching & limm loading
				std::list<const isa::InstructionDescription*> instructions = ins_formats[i->first];

				source_str << "switch(Instr_Code) {\n";
				for (std::list<const isa::InstructionDescription*>::const_iterator ins_ci = instructions.begin(); ins_ci != instructions.end(); ++ins_ci) {
					const isa::InstructionDescription *ins = *ins_ci;
					if (ins->Uses_PC_Constraints.size() == 0 && ins->EOB_Contraints.size() == 0 && ins->LimmCount == 0)
						continue;
					source_str << "case INST_" << ins->Name << ": {";

#ifdef ENABLE_LIMM_OPERATIONS
					if (ins->LimmCount != 0)
						source_str << "LimmBytes = " << (uint16_t) ins->LimmCount << ";\n";
					else
						source_str << "LimmBytes = 0;";
#endif

					//First, set up Uses_PC
					for (std::list<std::vector<isa::InstructionDescription::DecodeConstraint> >::const_iterator constraint_set = ins->Uses_PC_Constraints.begin(); constraint_set != ins->Uses_PC_Constraints.end(); ++constraint_set) {
						//if we come across a constraint set with zero constraints, then this instruction is always an end-of-block.
						if (constraint_set->size() == 0) {
							source_str << "SetUsesPC(); ";
							break;
						}

						source_str << "if(";
						bool first = true;
						for (std::vector<isa::InstructionDescription::DecodeConstraint>::const_iterator constraint = constraint_set->begin(); constraint != constraint_set->end(); ++constraint) {
							if (!first)
								source_str << " && ";
							first = false;
							if (constraint->Type == isa::InstructionDescription::Constraint_Equals)
								source_str << "(" << constraint->Field << " == " << constraint->Value << ")";
							else
								source_str << "(" << constraint->Field << " != " << constraint->Value << ")";
						}
						source_str << ")"
						           "SetUsesPC();\n";
					}

					//Now, set up End of block
					for (std::list<std::vector<isa::InstructionDescription::DecodeConstraint> >::const_iterator constraint_set = ins->EOB_Contraints.begin(); constraint_set != ins->EOB_Contraints.end(); ++constraint_set) {
						//if we come across a constraint set with zero constraints, then this instruction is always an end-of-block.
						if (constraint_set->size() == 0) {
							source_str << "SetEndOfBlock(); ";
							break;
						}

						source_str << "if(";
						bool first = true;
						for (std::vector<isa::InstructionDescription::DecodeConstraint>::const_iterator constraint = constraint_set->begin(); constraint != constraint_set->end(); ++constraint) {
							if (!first)
								source_str << " && ";
							first = false;
							if (constraint->Type == isa::InstructionDescription::Constraint_Equals)
								source_str << "(" << constraint->Field << " == " << constraint->Value << ")";
							else
								source_str << "(" << constraint->Field << " != " << constraint->Value << ")";
						}
						source_str << ")"
						           "SetEndOfBlock();";
					}
					source_str << "break; }\n";
				}
				source_str << "default : { ClearUsesPC();ClearEndOfBlock(); break; }\n }\n";

				source_str << "}\n\n";
			}

			source_str << "} }\n"; //namespaces
			return success;
		}

		bool ByteByByteDecodeGenerator::GenerateDecodeTree(DecodeNode& tree, std::stringstream &stream, int &i) const
		{
			//first, break the node at (fetch length) bits
			uint32_t fetch_length = Manager.GetArch().ISA->GetFetchLength();
			if ((tree.start_ptr % fetch_length) == 0) {
				tree.BreakUnconstrainedTransition(fetch_length);
				tree.BreakAllTransitions(fetch_length);
				tree.MergeCommonNodes();
			}

			uint32_t fetch_block = tree.start_ptr / fetch_length;

			stream << "//Node " << tree.node_number << "\n";

			//if this node falls on a (fetch length) boundary, and it has transitions, then fetch the next fetch unit
			if ((tree.start_ptr % fetch_length) == 0 && (tree.transitions.size() || tree.unconstrained_transition)) {
				stream << "{"
				       "uint32 efa;"
				       "uint32 loaded;"
				       "cpu.fetch" << fetch_length << "(pc + " << (fetch_block * fetch_length / 8) << ", loaded, efa, false);"
				       "insn_ir[" << fetch_block << "] = loaded;"
				       "}";
			}

			bool success = true;
			const arch::ArchDescription &Architecture = Manager.GetArch();

			//have we seen any transitions from this node? used to emit 'else' keywords
			bool seen_transitions = false;

			if (GetProperty("Debug") == "1")
				stream << "printf(\"Node %u\\n\"," << tree.node_number << ");\n";

			//if this is a leaf node
			if (tree.target) {
				//finish decoding the instruction fields
				stream << "Instr_Code = INST_" << tree.target->Name << ";\n";
				stream << "Decode_Format_" << tree.target->Format->Name << "(instr, cpu);\n";
#ifdef ENABLE_LIMM_OPERATIONS
				stream << "Instr_Length = " << tree.start_ptr / 8 << " + LimmBytes;\n";
#else
				stream << "Instr_Length = " << tree.start_ptr / 8 << ";\n";
#endif
				stream << "return;\n";
			} else {
				//otherwise look at the transitions from this node and sort them by their length
				std::map<uint8_t, std::list<DecodeTransition> > sorted_transitions;
				for (std::multimap<uint8_t, DecodeTransition>::iterator trans = tree.transitions.begin(); trans != tree.transitions.end(); ++trans) {
					if (sorted_transitions.find(trans->first) == sorted_transitions.end())
						sorted_transitions.insert(std::pair<uint8_t, std::list<DecodeTransition> >(trans->first, std::list<DecodeTransition>()));
					sorted_transitions.at(trans->first).push_back(trans->second);
				}

				//loop over groups in reverse order to get longest length first - essentially we implement longest-prefix-matching
				for (std::map<uint8_t, std::list<DecodeTransition> >::reverse_iterator group = sorted_transitions.rbegin(); group != sorted_transitions.rend(); ++group) {
					std::list<DecodeTransition> &list = group->second;

					//figure out which bits of the instruction code we should be looking at
					uint32_t low_bit = (Architecture.wordsize - tree.start_ptr - group->first);
					uint32_t high_bit = (Architecture.wordsize - 1 - tree.start_ptr);

					stream << "switch(";

					//if this transition is only one bit long, use a faster way of getting at that bit
					if (group->first == 1)
						stream << "(insn_ir[" << fetch_block << "] & BIT_LSB(" << high_bit << ")) >> " << high_bit << ")\n {\n";
					else
						stream << "UNSIGNED_BITS(insn_ir[" << fetch_block << "], " << high_bit << "," << low_bit << ")) \n{\n";

					//loop through the transitions of this group and emit their subtrees
					for (std::list<DecodeTransition>::iterator trans = list.begin(); trans != list.end(); ++trans) {
						stream << "case " << (trans->value) << ": { \n";

						success &= GenerateDecodeTree(*(trans->target), stream, i);

						stream << "\nbreak; }\n";
					}
					stream << "}\n";
				}

				//if we have an unconstrained transition (i.e., we're allowed to match 'anything')
				if (tree.unconstrained_transition) {
					if (seen_transitions)
						stream << "{\n";
					//emit an 'anything' subtree
					success &= GenerateDecodeTree(*(tree.unconstrained_transition->target), stream, i);
					if (seen_transitions)
						stream << "}\n";
					seen_transitions = true;
				}
			}
			return success;
		}
	}
}
#endif