/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "generators/FunctionalDecodeGenerator.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"
#include "DecodeTree.h"
#include "Util.h"

DEFINE_COMPONENT(gensim::generator::FunctionalDecodeGenerator, decode)
COMPONENT_INHERITS(decode, base_decode);

namespace gensim
{
	namespace generator
	{

		FunctionalDecodeGenerator::FunctionalDecodeGenerator(GenerationManager &man) : DecodeGenerator(man, "decode") {}

		bool FunctionalDecodeGenerator::EmitExtraClassMembers(util::cppformatstream &stream) const
		{
			return true;
		}

		bool FunctionalDecodeGenerator::GenerateDecodeHeader(util::cppformatstream &header_str) const
		{
			bool success = true;
			const arch::ArchDescription &Architecture = Manager.GetArch();
			const InterpretiveExecutionEngineGenerator *interpret = (const InterpretiveExecutionEngineGenerator*)Manager.GetComponentC(GenerationManager::FnInterpret);

			header_str << "/* Auto Generated Decoded Instruction Class for Arch " << Architecture.Name << " */\n";
			header_str << "#ifndef __ARCH_" << Architecture.Name << "_DECODE_INSTR_HEADER \n";
			header_str << "#define __ARCH_" << Architecture.Name << "_DECODE_INSTR_HEADER \n";

			header_str << "\n#include <define.h>\n\n";

			header_str << "#include <gensim/gensim_decode.h>\n";
			header_str << "#include <core/MemoryInterface.h>\n";
			header_str << "#include <core/thread/ThreadInstance.h>\n";
			header_str << "#include <queue>\n";
			header_str << "#include <utility>\n";

			header_str << "typedef uint8_t uint8;\n";
			header_str << "typedef uint16_t uint16;\n";
			header_str << "typedef uint32_t uint32;\n";
			header_str << "typedef uint64_t uint64;\n";
			header_str << "typedef int8_t sint8;\n";
			header_str << "typedef int16_t sint16;\n";
			header_str << "typedef int32_t sint32;\n";
			header_str << "typedef int64_t sint64;\n";
			header_str << "typedef int8_t sint8_t;\n";
			header_str << "typedef int16_t sint16_t;\n";
			header_str << "typedef int32_t sint32_t;\n";
			header_str << "typedef int64_t sint64_t;\n";


			header_str << "namespace gensim {\n";
			header_str << "namespace " << Architecture.Name << "{\n";

			header_str << "class Processor;\n";

			GenerateDecodeEnum(header_str);

			// generate class for decoded instruction
			header_str << "class " << GetProperty("class") << " : public gensim::BaseDecode\n{\n";

			header_str << "public:\n";

			header_str << "  using Instruction = " << GetProperty("class") << ";";

			header_str << "//Built in fields\n";
			// header_str << "\t" << GetProperty("class") << "_Enum\tInstr_Code;\n";

			// header_str << "\n//Instruction Fields\n";

			EmitInstructionFields(header_str);

			header_str << "\n";

			header_str << "uint32_t DecodeInstr(archsim::Address PC, uint32_t isa_mode, archsim::MemoryInterface &fetch_interface);";

			header_str << "void DecodeInstr(uint32_t, uint8_t);\n\n";

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				// generate type-specific decode functions if they exist
				for (std::map<std::string, isa::InstructionFormatDescription *>::const_iterator i = isa->Formats.begin(); i != isa->Formats.end(); ++i) {
					if (i->second->DecodeBehaviourCode.length() > 0) {
						if (GetProperty("InlineHints") == "1") header_str << "inline ";
						header_str << "void "
						           << "decode_behaviour_" << isa->ISAName << "_" << i->second->GetName() << "(";

						bool first = true;
						for (std::list<isa::InstructionFormatChunk>::const_iterator ifc = i->second->GetChunks().begin(); ifc != i->second->GetChunks().end(); ++ifc) {
							if (!ifc->generate_field && !ifc->is_constrained) {
								if (first) {
									header_str << util::Util::TypeString(ifc->length);
									first = false;
								} else {
									header_str << ", " << util::Util::TypeString(ifc->length);
								}
							}
						}

						header_str << ");\n";
					}
				}

				for (std::map<std::string, isa::InstructionFormatDescription *>::const_iterator i = isa->Formats.begin(); i != isa->Formats.end(); ++i) {
					if (GetProperty("InlineHints") == "1") header_str << "inline ";
					header_str << "void ";
					header_str << "Decode_Format_" << isa->ISAName << "_" << i->first << "(uint32_t);\n";
				}
			}

			if (interpret && interpret->GetProperty("StaticPredication") == "1") {
				for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
					const isa::ISADescription *isa = *II;
					header_str << "bool " << isa->ISAName << "_is_predicated();";
				}
			}

			EmitExtraClassMembers(header_str);

			header_str << "};\n\n";  // class

			header_str << "} }\n\n";  // namespaces

			header_str << "#endif\n";
			return success;
		}

		bool FunctionalDecodeGenerator::EmitDecodeFields(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const
		{
			// decode MSB to LSB
			int pos = 31;  // oh god oh god, TODO: figure out what our longest instruction length for this ISA is and put it here

			// decode each individual chunk
			for (const auto &chunk : format.GetChunks()) {
				if (chunk.is_constrained) {
					pos -= chunk.length;
					continue;
				}
				if (!chunk.generate_field) {
					stream << util::Util::TypeString(chunk.length) << " ";
				}
				if (isa.IsFieldOrthogonal(chunk.Name)) {
					pos -= chunk.length;
					continue;
				}
				assert(pos >= 0);
				assert(pos - chunk.length + 1 >= 0);
				if (chunk.length == 1)
					stream << chunk.Name << " = BITSEL(instr, " << pos << ");\n";
				else
					stream << chunk.Name << " = "
					       << "UNSIGNED_BITS(instr, " << pos << "," << pos - chunk.length + 1 << ");\n";
				pos -= chunk.length;
			}

			return true;
		}

		bool FunctionalDecodeGenerator::EmitEndOfBlockHandling(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const
		{

			// create code for end-of-block matching & limm loading

			stream << "switch(Instr_Code) {\n";
			for (const auto ins : format.GetInstructions()) {
				if (ins->Uses_PC_Constraints.size() == 0 && ins->EOB_Contraints.size() == 0 && ins->LimmCount == 0) continue;

				stream << "case INST_" << isa.ISAName << "_" << ins->Name << ": {";

#ifdef ENABLE_LIMM_OPERATIONS
				if (ins->LimmCount != 0)
					source_str << "LimmBytes = " << (uint16_t)ins->LimmCount << ";\n";
				else
					source_str << "LimmBytes = 0;";
#endif

				// First, set up Uses_PC
				for (std::list<std::vector<isa::InstructionDescription::DecodeConstraint> >::const_iterator constraint_set = ins->Uses_PC_Constraints.begin(); constraint_set != ins->Uses_PC_Constraints.end(); ++constraint_set) {
					// if we come across a constraint set with zero constraints, then this instruction is always an end-of-block.
					if (constraint_set->size() == 0) {
						stream << "SetUsesPC();";
						break;
					}

					stream << "if(";
					bool first = true;
					for (std::vector<isa::InstructionDescription::DecodeConstraint>::const_iterator constraint = constraint_set->begin(); constraint != constraint_set->end(); ++constraint) {
						if (!first) stream << " && ";
						first = false;
						switch(constraint->Type) {
							case isa::InstructionDescription::Constraint_Equals:
								stream << "(" << constraint->Field << " == " << constraint->Value << ")";
								break;
							case isa::InstructionDescription::Constraint_BitwiseAnd:
								stream << "((" << constraint->Field << " & " << constraint->Value << ") == " << constraint->Value << ")";
								break;
							case isa::InstructionDescription::Constraint_BitwiseXor:
								stream << "((" << constraint->Field << " ^ " << constraint->Value << ") != 0)";
								break;
							case isa::InstructionDescription::Constraint_NotEquals:
								stream << "(" << constraint->Field << " != " << constraint->Value << ")";
								break;
							default:
								assert(false);
						}
					}
					stream << ")"
					       "SetUsesPC();";
				}

				// Now, set up End of block
				for (std::list<std::vector<isa::InstructionDescription::DecodeConstraint> >::const_iterator constraint_set = ins->EOB_Contraints.begin(); constraint_set != ins->EOB_Contraints.end(); ++constraint_set) {
					// if we come across a constraint set with zero constraints, then this instruction is always an end-of-block.
					if (constraint_set->size() == 0) {
						stream << "SetEndOfBlock();";
						break;
					}

					stream << "if(";
					bool first = true;
					for (std::vector<isa::InstructionDescription::DecodeConstraint>::const_iterator constraint = constraint_set->begin(); constraint != constraint_set->end(); ++constraint) {
						if (!first) stream << " && ";
						first = false;
						if (constraint->Type == isa::InstructionDescription::Constraint_Equals)
							stream << "(" << constraint->Field << " == " << constraint->Value << ")";
						else if (constraint->Type == isa::InstructionDescription::Constraint_BitwiseAnd)
							stream << "((" << constraint->Field << " & " << constraint->Value << ") == " << constraint->Value << ")";
						else
							stream << "(" << constraint->Field << " != " << constraint->Value << ")";
					}
					stream << ")"
					       "SetEndOfBlock();";
				}
				stream << "break; }\n";
			}
			stream << "default : { ClearUsesPC();ClearEndOfBlock(); break; }\n }\n";
			return true;
		}

		bool FunctionalDecodeGenerator::EmitDecodeBehaviourCall(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const
		{
			if (format.DecodeBehaviourCode.length() > 0) {
				std::stringstream param_str;

				bool first = true;
				// decode each individual chunk
				for (const auto &chunk : format.GetChunks()) {
					if (chunk.is_constrained) {
						continue;
					}
					if (!chunk.generate_field) {
						if (first) {
							first = false;
							param_str << chunk.Name;
						} else {
							param_str << ", " << chunk.Name;
						}
					}
				}

				if (format.DecodeBehaviourCode.length() > 0) {
					stream << "\tdecode_behaviour_" << isa.ISAName << "_" << format.GetName() << "(" << param_str.str() << ");\n";
				}
			}

			return true;
		}

		bool FunctionalDecodeGenerator::GenerateFormatDecoder(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const
		{
			const InterpretiveExecutionEngineGenerator *interpret = (const InterpretiveExecutionEngineGenerator *)Manager.GetComponentC(GenerationManager::FnInterpret);

			stream << "void " << GetProperty("class") << "::Decode_Format_" << isa.ISAName << "_" << format.GetName() << "(uint32_t instr)\n{";

			EmitDecodeFields(isa, format, stream);
			EmitDecodeBehaviourCall(isa, format, stream);

			// if we're interpreting with static predication, set the predication here

			if (interpret && interpret->GetProperty("StaticPredication") == "1") {
				stream << "if(" << isa.ISAName << "_is_predicated()) SetIsPredicated(); else ClearIsPredicated();";
			}

			EmitEndOfBlockHandling(isa, format, stream);

			stream << "}\n\n";

			return true;
		}

		bool FunctionalDecodeGenerator::GenerateDecodeSource(util::cppformatstream &source_str) const
		{
			// Emit some preamble: includes, namespaces etc

			bool success = true;
			const arch::ArchDescription &Architecture = Manager.GetArch();
			const InterpretiveExecutionEngineGenerator *interpret = (const InterpretiveExecutionEngineGenerator *)Manager.GetComponentC(GenerationManager::FnInterpret);
			source_str << "#include <stdio.h>\n\n";
			source_str << "#include \"decode.h\"\n\n";

			if(Manager.GetComponentC(Manager.FnInterpret))
				source_str << "#include \"processor.h\"\n";

			source_str << "#define INTERP\n";
			source_str << "#undef PREFIX\n";
			source_str << "#define PREFIX cpu.\n";
			source_str << "#include <gensim/gensim_processor_api.h>\n";
			source_str << "#undef INTERP\n";

			source_str << "namespace gensim { \n namespace " << Architecture.Name << "{ \n";

			// End of preamble, here comes the good stuff

			// If for each ISA, if instructions are statically predicated, emit an 'is_predicated' function for that isa
			if (interpret && interpret->GetProperty("StaticPredication") == "1") {
				for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
					const isa::ISADescription *isa = *II;
					source_str << "bool " << GetProperty("class") << "::" << isa->ISAName << "_is_predicated() { " << GetProperty("class") << "*curr_interp_insn = this;";
					source_str << isa->GetBehaviourAction("is_predicated");
					source_str << "}";
				}
			}

			// Start emitting actual instruction decode code
			source_str << "uint32_t " << GetProperty("class") << "::DecodeInstr(archsim::Address pc, uint32_t _isa_mode, archsim::MemoryInterface &fetch)\n{\n";

			// Start by mapping format names to instruction descriptions
			std::map<std::string, std::list<const isa::InstructionDescription *> > ins_formats;

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (std::map<std::string, isa::InstructionFormatDescription *>::const_iterator i = isa->Formats.begin(); i != isa->Formats.end(); ++i) {
					ins_formats[i->second->GetName()] = std::list<const isa::InstructionDescription *>();
				}

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator ci = isa->Instructions.begin(); ci != isa->Instructions.end(); ++ci) {
					ins_formats[ci->second->Format->GetName()].push_back(ci->second);
				}
			}

			source_str << "uint32_t instr = 0;";
			source_str << "archsim::MemoryResult ecause;\n";
			source_str << "uint32_t chunk32 = 0;\n";
			source_str << "uint16_t chunk16 = 0;\n";
			source_str << "uint8_t chunk8 = 0;\n";

			// Fetch the instruction data depending on which ISA we're decoding for
			source_str << "switch(_isa_mode) {";

			int maxarchinsnlen = 0;
			for(const auto &isa : Architecture.ISAs) {
				if(maxarchinsnlen < isa->GetMaxInstructionLength()) maxarchinsnlen = isa->GetMaxInstructionLength();
			}

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;
				int maxlen = isa->GetMaxInstructionLength();

				if(maxlen > 32) {
					fprintf(stderr, "Do not currently support instructions longer than 32 bits\n");
					return false;
				}

				source_str << "case ISA_MODE_" << isa->ISAName << ":";
				int fetched = 0;

				while(fetched < maxlen) {
					source_str << "ecause = fetch.Read" << isa->GetFetchLength() << "(pc + " << fetched/8 << ", chunk" << isa->GetFetchLength() << ");";
					source_str << "if(ecause != archsim::MemoryResult::OK) return 1;";
					source_str << "instr |= chunk" << isa->GetFetchLength() << ";";

					fetched += isa->GetFetchLength();

					//If there's still potentially more instruction to fetched, make some space for it
					if(fetched < maxlen)
						source_str << "instr <<= " << isa->GetFetchLength() << ";";
				}

				//Shift up the remaining bits if we have ISAs with different insn lengths
				if(fetched < maxarchinsnlen) source_str << "instr <<= " << maxarchinsnlen - fetched << ";";

				source_str << "break;";
			}

			source_str << "default:";
			source_str << "ecause = fetch.Read32(pc, instr); if(ecause != archsim::MemoryResult::OK) return 1;";
			source_str << "break;";
			source_str << "}";

			// recursively emit decode statements for the tree
			source_str << "DecodeInstr(instr, _isa_mode);"
			           "return 0;";
			source_str << "}";


			source_str << "void " << GetProperty("class") << "::DecodeInstr(uint32_t instr, uint8_t _isa_mode)\n{\n";

			source_str << "   Instr_Code = __INST_CODE_INVALID__;\n"; //(" << GetProperty("class") << "_Enum)(unsigned long)(-1);\n";
			source_str << "   ClearEndOfBlock();";
			source_str << "   ClearUsesPC();\n";
			source_str << "   ClearIsPredicated();";
			source_str << "   SetIR(instr);\n";
#ifdef ENABLE_LIMM_OPERATIONS
			source_str << "   LimmPtr = 0;\n";
			source_str << "   LimmBytes = 0;\n";
#endif

			int n = 0;
			// Now actually decode the instruction
			source_str << "switch (_isa_mode) {\n";
			for (std::map<const isa::ISADescription *, DecodeNode *>::const_iterator DI = decode_trees.begin(), DE = decode_trees.end(); DI != DE; ++DI) {
				source_str << "case ISA_MODE_" << DI->first->ISAName << ": {\n";
				success &= GenerateDecodeTree(*DI->first, *DI->second, source_str, n);
				source_str << "} break;\n";
			}

			source_str << "}\n";
			source_str << "}\n\n";

			// generate type-specific decode functions if they exist

			for (std::list<isa::ISADescription *>::const_iterator II = Architecture.ISAs.begin(), IE = Architecture.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (std::map<std::string, isa::InstructionFormatDescription *>::const_iterator i = isa->Formats.begin(); i != isa->Formats.end(); ++i) {
					isa::InstructionFormatDescription *ins = i->second;
					if (i->second->DecodeBehaviourCode.length() > 0) {
						source_str << "void " << GetProperty("class") << "::decode_behaviour_" << isa->ISAName << "_" << i->second->GetName() << "(";

						bool first = true;
						for (std::list<isa::InstructionFormatChunk>::const_iterator j = ins->GetChunks().begin(); j != ins->GetChunks().end(); ++j) {
							if (!j->generate_field && !j->is_constrained) {
								if (first) {
									source_str << util::Util::TypeString(j->length) << " " << j->Name;
									first = false;
								} else
									source_str << ", " << util::Util::TypeString(j->length) << " " << j->Name;
							}
						}
						source_str << ")\n" << i->second->DecodeBehaviourCode << "\n\n";
					}
				}

				// generate decode functions for each instruction format
				for (std::map<std::string, isa::InstructionFormatDescription *>::const_iterator i = isa->Formats.begin(); i != isa->Formats.end(); ++i) {
					GenerateFormatDecoder(*isa, *i->second, source_str);
				}
			}

			source_str << "} }\n";  // namespaces
			return success;
		}

		bool FunctionalDecodeGenerator::GenerateDecodeLeaf(const isa::ISADescription &isa, const isa::InstructionDescription &insn, util::cppformatstream &stream) const
		{
			// Make sure that all of the inequality decode constraints are satisfied
			if(insn.Decode_Constraints.size()) {
				for (const auto &constraint : insn.Decode_Constraints.front()) {
					if (constraint.Type == isa::InstructionDescription::Constraint_NotEquals) stream << "// should make sure that " << constraint.Field << " != " << constraint.Value << std::endl;
				}
			}

			// finish decoding the instruction fields
			stream << "Instr_Code = INST_" << isa.ISAName << "_" << insn.Name << ";\n";
			// TODO: WARNING!! FILL THIS IN
			stream << "isa_mode = " << isa.isa_mode_id << ";\n";
			stream << "Decode_Format_" << isa.ISAName << "_" << insn.Format->GetName() << "(instr);\n";
#ifdef ENABLE_LIMM_OPERATIONS
			stream << "Instr_Length = " << tree.start_ptr / 8 << " + LimmBytes;\n";
#else
			stream << "Instr_Length = " << insn.Format->GetLength() / 8 << ";\n";
#endif

			stream << "return;\n";

			return true;
		}

		bool FunctionalDecodeGenerator::GenerateDecodeTree(const isa::ISADescription &isa, DecodeNode &tree, util::cppformatstream &stream, int &i) const
		{
			bool success = true;
			const arch::ArchDescription &Architecture = Manager.GetArch();

			// have we seen any transitions from this node? used to emit 'else' keywords
			bool seen_transitions = false;

			stream << "//Node " << tree.node_number << "\n";

			if (GetProperty("Debug") == "1") stream << "printf(\"Node %u\\n\"," << tree.node_number << ");\n";

			// if this is a leaf node
			if (tree.target) {
				GenerateDecodeLeaf(isa, *tree.target, stream);
			} else {
				// otherwise look at the transitions from this node and sort them by their length
				std::map<uint8_t, std::list<DecodeTransition> > sorted_transitions;
				for (std::multimap<uint8_t, DecodeTransition>::iterator trans = tree.transitions.begin(); trans != tree.transitions.end(); ++trans) {
					if (sorted_transitions.find(trans->first) == sorted_transitions.end()) sorted_transitions.insert(std::pair<uint8_t, std::list<DecodeTransition> >(trans->first, std::list<DecodeTransition>()));
					sorted_transitions.at(trans->first).push_back(trans->second);
				}

				// loop over groups in reverse order to get longest length first - essentially we implement longest-prefix-matching
				for (std::map<uint8_t, std::list<DecodeTransition> >::reverse_iterator group = sorted_transitions.rbegin(); group != sorted_transitions.rend(); ++group) {
					std::list<DecodeTransition> &list = group->second;

					uint32_t bits = Architecture.GetMaxInstructionSize();
					// figure out which bits of the instruction code we should be looking at
					uint32_t low_bit = (bits - tree.start_ptr - group->first);
					uint32_t high_bit = (bits - 1 - tree.start_ptr);

					stream << "switch(";

					// if this transition is only one bit long, use a faster way of getting at that bit
					if (group->first == 1)
						stream << "(instr & BIT_LSB(" << high_bit << ")) >> " << high_bit << ")\n {\n";
					else
						stream << "UNSIGNED_BITS(instr, " << high_bit << "," << low_bit << ")) \n{\n";

					// loop through the transitions of this group and emit their subtrees
					for (std::list<DecodeTransition>::iterator trans = list.begin(); trans != list.end(); ++trans) {
						stream << "case " << (trans->value) << ": { \n";

						success &= GenerateDecodeTree(isa, *(trans->target), stream, i);

						stream << "\nbreak; }\n";
					}
					stream << "}\n";
				}

				// if we have an unconstrained transition (i.e., we're allowed to match 'anything')
				if (tree.unconstrained_transition) {
					if (seen_transitions) stream << "{\n";
					// emit an 'anything' subtree
					success &= GenerateDecodeTree(isa, *(tree.unconstrained_transition->target), stream, i);
					if (seen_transitions) stream << "}\n";
					seen_transitions = true;
				}
			}
			return success;
		}
	}  // namespace generator
}  // namespace gensim
