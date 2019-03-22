/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>

#include <antlr3.h>
#include <antlr-ver.h>
#include <archcasm/archcasmLexer.h>
#include <archcasm/archcasmParser.h>

#include "generators/DisasmGenerator.h"
#include "isa/ISADescription.h"

#include "antlr-ver.h"

DEFINE_COMPONENT(gensim::generator::DisasmGenerator, disasm);
COMPONENT_OPTION(disasm, class, "Disasm", "Set the class name of the disassembly component.")
COMPONENT_OPTION(disasm, InlineHints, "0", "Emit inline hints on disassembly functions, increasing code size but possibly improving performance")

namespace gensim
{
	namespace generator
	{

		bool AsmConstraintComparison::operator()(const isa::AsmDescription * const&a, const isa::AsmDescription * const &b) const
		{
			return (a->constraints.size() < b->constraints.size()) ^ rev;
		}

// function for expression function processing

		std::string modifier_decode_function(std::string this_str, std::string fnname, int argc, const util::expression **argv)
		{
			std::stringstream output;
			output << "modifier_decode_" << fnname << "(";
			bool first = true;
			for (int i = 0; i < argc; ++i) {
				if (!first) output << ",";
				output << argv[i]->ToString(this_str, &modifier_decode_function);
			}
			output << ", instr, pc)";
			return output.str();
		}

		bool DisasmGenerator::Generate() const
		{
			return GenerateDisasm(Manager.GetTarget());
		}

		DisasmGenerator::~DisasmGenerator() {}

		bool DisasmGenerator::GenerateDisasmHeader(std::stringstream &header_str) const
		{
			bool success = true;
			const arch::ArchDescription &arch = Manager.GetArch();
			const GenerationComponent *decode = Manager.GetComponentC(GenerationManager::FnDecode);

			header_str << "/* Auto generated disassembler class for Arch " << Manager.GetArch().Name << " */\n";

			header_str << "#ifndef __ARCH_" << Manager.GetArch().Name << "_DISASM_HEADER \n";
			header_str << "#define __ARCH_" << Manager.GetArch().Name << "_DISASM_HEADER \n";

			header_str << "\n#include <define.h>\n\n";
			header_str << "#include <string>\n";
			header_str << "#include <sstream>\n";
			header_str << "#include <iostream> // for error messages \n";
			header_str << "#include <cstdlib> // for abort()\n";

			header_str << "#include <gensim/gensim_disasm.h>\n";

			header_str << "#include \"decode.h\"\n\n";

			// header_str << "extern \"C\" { int StaticDisasmInstr(char *target, uint32_t IR, uint32_t PC); }";

			header_str << "namespace gensim {\n";
			header_str << "namespace " << arch.Name << "{\n";

			header_str << "class " << GetProperty("class") << " : public gensim::BaseDisasm \n{\n";

			header_str << "private:\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				// output assembler mappings
				for (std::map<std::string, isa::AsmMapDescription>::const_iterator i = isa->Mappings.begin(); i != isa->Mappings.end(); ++i) {
					header_str << "static std::string Map_" << isa->ISAName << "_" << i->second.Name << "[];\n";
				}

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					header_str << "\tstd::string ";
					if (GetProperty("InlineHints") == "1") header_str << "inline ";

					header_str << "Disasm_" << isa->ISAName << "_" << i->second->Name << "(const " << decode->GetProperty("class") << "&, archsim::Address pc) const;\n";
				}
			}

			header_str << "public:\n";

			header_str << "\tstd::string DisasmInstr(const gensim::BaseDecode&, archsim::Address pc) const;\n\n";
			header_str << "\tstd::string GetInstrName(uint32_t opcode) const;\n";
			header_str << "\tstatic std::string InstructionNames[];\n";

			header_str << "};\n";

			header_str << "} }\n";  // namespaces

			header_str << "#endif\n";
			return success;
		}

		bool DisasmGenerator::GenerateDisasmSource(std::stringstream &source_str) const
		{
			bool success = true;
			source_str << "/* Auto generated disassembler class for Arch " << Manager.GetArch().Name << " */\n";

			const GenerationComponent *decode = Manager.GetComponentC(GenerationManager::FnDecode);
			const arch::ArchDescription &arch = Manager.GetArch();

			source_str << "#include <string>\n";
			source_str << "#include <sstream>\n";

			source_str << "#include \"decode.h\"\n";
			source_str << "#include \"disasm.h\"\n";

			source_str << "#include <cstring>\n";

			source_str << "namespace gensim {\n";
			source_str << "namespace " << arch.Name << "{\n";

			// Emit GetInstrName
			source_str << "std::string " << GetProperty("class") << "::GetInstrName(uint32_t opcode) const\n";
			source_str << "{\n";

			source_str << "switch(opcode)\n {\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator DMI = isa->Instructions.begin(), DME = isa->Instructions.end(); DMI != DME; ++DMI) {
					source_str << "case INST_" << isa->ISAName << "_" << DMI->second->Name << ": return \"" << DMI->second->Name << "\"; break;\n";
				}
			}

			source_str << "}\n";
			source_str << "return \"UNKNOWN\";";
			source_str << "}\n";

			// include modifiers file
			// TODO: Review the multi-isa stuff here
			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				if (isa->HasProperty("Modifiers")) util::Util::InlineFile(source_str, isa->GetProperty("Modifiers"));
			}

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				// emit assembly mappings
				for (std::map<std::string, isa::AsmMapDescription>::const_iterator i = isa->Mappings.begin(); i != isa->Mappings.end(); ++i) {
					source_str << "std::string " << GetProperty("class") << "::Map_" << isa->ISAName << "_" << i->second.Name << "[] = {\n";

					for (int map = i->second.GetMin(); map <= i->second.GetMax(); ++map) {
						source_str << "\t\"";
						if (i->second.mapping.find(map) != i->second.mapping.end()) {
							source_str << i->second.mapping.at(map);
						}
						source_str << "\",\n";
					}
					source_str << "};\n";
				}
				source_str << "\n";
			}

			source_str << "std::string " << GetProperty("class") << "::DisasmInstr(const gensim::BaseDecode &baseinstr, archsim::Address pc) const\n";
			source_str << "{\n";

			source_str << "const " << decode->GetProperty("class") << " &instr = static_cast<const " << decode->GetProperty("class") << "&>(baseinstr);\n";

			source_str << "switch(instr.Instr_Code)\n {\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					if (decode->HasProperty("Classful"))
						source_str << "case INST_" << isa->ISAName << "_" << i->second->Name << ": return Disasm_" << isa->ISAName << "_" << i->second->Name << "(static_cast<Decode_" << i->second->Format->GetName() << "&>(instr), pc);break;\n";
					else
						source_str << "case INST_" << isa->ISAName << "_" << i->second->Name << ": return Disasm_" << isa->ISAName << "_" << i->second->Name << "(instr, pc);break;\n";
				}
			}

			source_str << "}\n";
			source_str << "return \"UNKNOWN\";";
			source_str << "}\n";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				// generate dissassembly functions
				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					success &= GenerateInstrDisasm(source_str, *isa, *i->second);
				}
			}

			source_str << "} }\n";  // namespaces
			return success;
		}

		bool DisasmGenerator::GenerateInstrDisasm(std::stringstream &source_str, const isa::ISADescription &isa, const isa::InstructionDescription &instr) const
		{
			bool success = true;
			const GenerationComponent *decode = Manager.GetComponentC(GenerationManager::FnDecode);
			const arch::ArchDescription &arch = Manager.GetArch();

			if (decode->HasProperty("Classful"))
				source_str << "std::string " << GetProperty("class") << "::Disasm_" << isa.ISAName << "_" << instr.Name << "(const " << decode->GetProperty("class_of_" + instr.Format->GetName()) << " &instr, archsim::Address pc) const \n{\n";
			else
				source_str << "std::string " << GetProperty("class") << "::Disasm_" << isa.ISAName << "_" << instr.Name << "(const " << decode->GetProperty("class") << " &instr, archsim::Address pc) const \n{\n";

			source_str << "std::stringstream str; uint32_t map_val;\n";

			if (instr.Disasms.empty()) {
				fprintf(stderr, "No disassembly provided for instruction %s\n", instr.Name.c_str());
				source_str << "str << \"" << instr.Name << "\";";
				source_str << "return str.str();\n";
			}

			std::list<isa::AsmDescription*> sorted_list(instr.Disasms);
			sorted_list.sort(AsmConstraintComparison(true));

			for (std::list<isa::AsmDescription*>::const_iterator i_i = sorted_list.begin(); i_i != sorted_list.end(); ++i_i) {
				isa::AsmDescription *i = *i_i;
				// first of all, we need to match the constraints
				if (i->constraints.size() > 0) {
					source_str << "if(";
					bool first = true;
					for (std::map<std::string, const util::expression *>::const_iterator constraint = i->constraints.begin(); constraint != i->constraints.end(); ++constraint) {
						if (!first) source_str << " && ";
						source_str << "(instr." << constraint->first << " == " << constraint->second->ToString("instr", &modifier_decode_function) << ")";
						first = false;
					}
					source_str << ") \n {\n";
				}

				// now, decode the format string and output

				// do all the ANTLR parsing setup and parse the format string
				pANTLR3_INPUT_STREAM pts = antlr3NewAsciiStringCopyStream((uint8_t *)i->format.c_str(), i->format.length(), (uint8_t *)"stream");
				parchcasmLexer lexer = archcasmLexerNew(pts);
				pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));

				parchcasmParser parser = archcasmParserNew(tstream);

				archcasmParser_asmStmt_return asm_stmt = parser->asmStmt(parser);

				pANTLR3_BASE_TREE root_node = asm_stmt.tree;

				// now loop through the parsed components and output code to emit them
				uint32_t curr_arg = 0;
				for (uint32_t childNum = 0; childNum < root_node->getChildCount(root_node); childNum++) {
					pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)root_node->getChild(root_node, childNum);
					// each node is either an ARG or just text
					if (child->getType(child) == ASM_ARG) {
						if (curr_arg >= i->args.size()) {
							fprintf(stderr, "Too few disassembly arguments provided to disassembly for instruction %s\n", instr.Name.c_str());
							success = false;
							return success;
						}

						// if this node is an ARG, grab the identifier which names either a map or a builtin map (imm or exp)
						pANTLR3_BASE_TREE name_child = (pANTLR3_BASE_TREE)child->getChild(child, 0);
						std::string map_name((char *)name_child->getText(name_child)->chars);

						// identifiers can be affected by modifiers (e.g. ... for reg lists). we can tell if a node is modified
						// by looking at how many children it has.
						if (child->getChildCount(child) == 1) { // if this arg is unmodified then process it immediately
							// if this is an imm or an exp (handled identically) then output the expression to generate that imm or exp
							if (!map_name.compare("imm") || !map_name.compare("exp") || !map_name.compare("reladdr") || !map_name.compare("hex32") || !map_name.compare("hex64")) {

								if (!map_name.compare("reladdr")) {
									source_str << "str << std::hex << (uint32_t)(pc.Get() + (sint32_t)(uint32_t)(" << i->args[curr_arg]->ToString("instr", &modifier_decode_function) << ")) << std::dec;\n";
								} else {
									source_str << "str << (uint32_t)(" << i->args[curr_arg]->ToString("instr", &modifier_decode_function) << ");\n";
								}
							} else {
								// otherwise look up the map (generating an error if it is invalid) and output code to generate a map lookup
								std::map<std::string, isa::AsmMapDescription>::const_iterator map_it = isa.Mappings.find(map_name);
								if (map_it == isa.Mappings.end()) {
									// TODO: throw an exception here instead
									fprintf(stderr, "Attempted to use an undefined asm map %s for insn %s\n", map_name.c_str(), i->instruction_name.c_str());
									return false;
								}
								source_str << "map_val = " << map_it->second.GetMin() << " + " << i->args[curr_arg]->ToString("instr", modifier_decode_function) << ";"
								           "if(map_val < " << map_it->second.mapping.size() << ")"
								           "str << " << GetProperty("class") << "::Map_" << isa.ISAName << "_" << map_name << "[map_val];\n"
								           "else str << \"???\";";
							}
						} else {
							// this node is modified by some modifier (... for reglist or L,H,A,R for value modifiers)
							pANTLR3_BASE_TREE mod_node = (pANTLR3_BASE_TREE)child->getChild(child, 1);  // get the main modifier node

							if (!map_name.compare("imm") || !map_name.compare("exp") || !map_name.compare("reladdr")) {
								if (mod_node->getType(mod_node) != ASM_MODIFIER) {
									fprintf(stderr, "Known values can only be modified by modifier functions\n");
									return false;
								}
								pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)mod_node->getChild(mod_node, 0);
								source_str << "str << modifier_decode_" << nameNode->getText(nameNode)->chars << "(" << i->args[curr_arg]->ToString("instr", &modifier_decode_function) << ", instr, pc);\n";
							} else {
								// look up the map we are mapping to
								std::map<std::string, isa::AsmMapDescription>::const_iterator map_it = isa.Mappings.find(map_name);
								if (map_it == isa.Mappings.end()) {
									// error
									fprintf(stderr, "Attempted to use an undefined asm map %s for insn %s\n", map_name.c_str(), i->instruction_name.c_str());
									return false;
								}
								isa::AsmMapDescription map = map_it->second;

								switch (mod_node->getType(mod_node)) {
									case ASM_BIT_LIST: {
										source_str << "uint32_t val = " << i->args[curr_arg]->ToString("instr", &modifier_decode_function) << ";\n bool first = true;\n";
										// loop from low to high bits. a register map value represents the bit representing that register
										for (int bit = map.GetMin(); bit <= map.GetMax(); bit++) {
											uint32_t val = 1 << bit;
											source_str << "if(val & " << val << ")\n";
											source_str << "{\n";
											source_str << "   if(!first) str << \", \";\n";
											source_str << "str << " << GetProperty("class") << "::Map_" << isa.ISAName << "_" << map_name << "[" << (bit - map.GetMin()) << "];\n";
											source_str << "first = false;\n";
											source_str << "}\n";
										}
										break;
									}
									default: {
										fprintf(stderr, "Modifier is not currently supported for insn %s\n", i->instruction_name.c_str());
										return false;
									}
								}
							}
						}
						curr_arg++;
					} else {
						source_str << "str << \"" << child->getText(child)->chars << "\";\n";
					}
				}

				parser->free(parser);
				tstream->free(tstream);
				lexer->free(lexer);
				pts->free(pts);

				source_str << "return str.str();\n";
				if (i->constraints.size() > 0) {

					source_str << "}\n";
				}
			}
			source_str << "return \"no disassembly for " << instr.Name << "\";";
			source_str << "}\n";
			return success;
		}

		bool DisasmGenerator::GenerateDisasm(std::string prefix) const
		{
			bool success = true;
			std::stringstream header_str, source_str;

			success &= GenerateDisasmHeader(header_str);
			success &= GenerateDisasmSource(source_str);

			if (success) {
				WriteOutputFile("disasm.h", header_str);
				WriteOutputFile("disasm.cpp", source_str);
			}
			return success;
		}

		const std::vector<std::string> DisasmGenerator::GetSources() const
		{
			std::vector<std::string> sources;
			sources.push_back("disasm.cpp");
			return sources;
		}

	}  // namespace generator

}  // namespace gensim
