/**
 * generators/Captive/DisasmGenerator.cpp
 *
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
#include "generators/GenerationManager.h"
#include "generators/DisasmGenerator.h"
#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "isa/InstructionFormatDescription.h"

#include <antlr3.h>
#include <antlr-ver.h>
#include <archcasm/archcasmLexer.h>
#include <archcasm/archcasmParser.h>

using namespace gensim::generator;

namespace gensim
{
	namespace generator
	{
		namespace captive
		{

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

			class DisasmGenerator : public GenerationComponent
			{
			public:
				DisasmGenerator(GenerationManager &man) : GenerationComponent(man, "captive_disasm") { }
				virtual ~DisasmGenerator() { }

				std::string GetFunction() const override
				{
					return "disasm";
				}

				const std::vector<std::string> GetSources() const override
				{
					return std::vector<std::string>();
				}

				bool Generate() const override
				{
					bool success = true;
					util::cppformatstream header_str;
					util::cppformatstream source_str;

					// generate the source streams
					success &= GenerateDisasmHeader(header_str);
					success &= GenerateDisasmSource(source_str);

					if (success) {
						WriteOutputFile(Manager.GetArch().Name + "-disasm.h", header_str);
						WriteOutputFile(Manager.GetArch().Name + "-disasm.cpp", source_str);
					}

					return success;
				}

			private:
				inline std::string ClassNameForDisasm() const
				{
					return Manager.GetArch().Name + "_disasm";
				}

				inline std::string ClassNameForFormatDecoder(const isa::InstructionFormatDescription& fmt) const
				{
					return Manager.GetArch().Name + "_decode_" + fmt.GetISA().ISAName + "_" + fmt.GetName();
				}

				inline std::string ClassNameForDecoder() const
				{
					return Manager.GetArch().Name + "_decode";
				}

				inline std::string EnumElementForInstruction(const isa::InstructionDescription& insn) const
				{
					return Manager.GetArch().Name + "_" + insn.Format->GetISA().ISAName + "_" + insn.Name;
				}

				bool GenerateDisasmHeader(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					str << "#ifndef __" << arch.Name << "_DISASM_H__\n";
					str << "#define __" << arch.Name << "_DISASM_H__\n";

					str << "#include <define.h>\n";
					str << "#include <disasm.h>\n";
					str << "#include <" << arch.Name << "-decode.h>\n";

					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace " << arch.Name << " {";

					str << "class " << ClassNameForDisasm() << " : public Disasm {";
					str << "public:\n";
					str << "const char *disassemble(uint64_t pc, const uint8_t *data) override;";

					str << "private:\n";

					for (auto isa : arch.ISAs) {
						for (const auto& map : isa->Mappings) {
							str << "static const char *_map_" << isa->ISAName << "_" << map.first << "[];";
						}
					}

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							str << "void disassemble_" << isa->ISAName << "_" << insn.first << "(uint64_t, const " << ClassNameForFormatDecoder(*insn.second->Format) << "&);";
						}
					}

					str << "};";

					str << "}";
					str << "}";
					str << "}";

					str << "#endif";

					return true;
				}

				bool GenerateDisasmSource(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					str << "#include <" << arch.Name << "-disasm.h>\n";
					str << "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n";

					str << "using namespace captive::arch::" << arch.Name << ";";

					for (auto isa : arch.ISAs) {
						for (const auto& map : isa->Mappings) {
							str << "const char *" << ClassNameForDisasm() << "::_map_" << isa->ISAName << "_" << map.first << "[] = {";

							for (auto entry : map.second.mapping) {
								str << "\"" << entry.second << "\",";
							}

							str << "};";
						}
					}

					str << "const char *" << ClassNameForDisasm() << "::disassemble(uint64_t pc, const uint8_t *data)";
					str << "{";
					str << "const " << ClassNameForDecoder() << "& insn = (const " << ClassNameForDecoder() << "&)*((const " << ClassNameForDecoder() << "*)data);";
					str << "clear();";
					str << "switch (insn.opcode) {";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							str << "case " << ClassNameForDecoder() << "::" << EnumElementForInstruction(*insn.second) << ":";
							str << "disassemble_" << isa->ISAName << "_" << insn.first << "(pc, (const " << ClassNameForFormatDecoder(*insn.second->Format) << "&)insn);";
							str << "break;";
						}
					}

					str	<< "default: append_str(\"???\"); break;";

					str << "}";
					str << "return buffer();";

					str << "}";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							GenerateInstrDisasm(str, *isa, *insn.second);
						}
					}

					return true;
				}

				bool GenerateInstrDisasm(util::cppformatstream &str, const isa::ISADescription &isa, const isa::InstructionDescription &insn) const
				{
					bool success = true;
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "void " << ClassNameForDisasm() << "::disassemble_" << isa.ISAName << "_" << insn.Name << "(uint64_t pc, const " << ClassNameForFormatDecoder(*insn.Format) << "& insn)";
					str << "{ uint32_t map_val = 0;";

					if (insn.Disasms.empty()) {
						fprintf(stderr, "No disassembly provided for instruction %s\n", insn.Name.c_str());
						str << "append_str(\"" << insn.Name << "\");";
						str << "}";
						return true;
					}

					std::list<isa::AsmDescription *> sorted_list(insn.Disasms);
					sorted_list.sort(gensim::generator::AsmConstraintComparison(true));

					for (const auto& desc : sorted_list) {
						// first of all, we need to match the constraints
						if (desc->constraints.size() > 0) {
							str << "if (";

							bool first = true;
							for (auto constraint : desc->constraints) {
								if (!first) {
									str << " && ";
								} else {
									first = false;
								}

								str << "(insn." << constraint.first << " == " << constraint.second->ToString("insn", &modifier_decode_function) << ")";
							}

							str << ") {";
						}

						// now, decode the format string and output

						// do all the ANTLR parsing setup and parse the format string
						pANTLR3_INPUT_STREAM pts = antlr3NewAsciiStringCopyStream((uint8_t *)desc->format.c_str(), desc->format.length(), (uint8_t *)"stream");
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
								if (curr_arg >= desc->args.size()) {
									fprintf(stderr, "Too few disassembly arguments provided to disassembly for instruction %s\n", insn.Name.c_str());
									return false;
								}

								// if this node is an ARG, grab the identifier which names either a map or a builtin map (imm or exp)
								pANTLR3_BASE_TREE name_child = (pANTLR3_BASE_TREE)child->getChild(child, 0);
								std::string map_name((char *)name_child->getText(name_child)->chars);

								// identifiers can be affected by modifiers (e.g. ... for reg lists). we can tell if a node is modified
								// by looking at how many children it has.
								if (child->getChildCount(child) == 1) { // if this arg is unmodified then process it immediately
									// if this is an imm or an exp (handled identically) then output the expression to generate that imm or exp
									if (!map_name.compare("imm") || !map_name.compare("exp") || !map_name.compare("reladdr") || !map_name.compare("hex64") || !map_name.compare("hex32")) {

										if (!map_name.compare("reladdr")) {
											str << "append_hex((uint64_t)((int64_t)pc + (int64_t)(uint64_t)(" << desc->args[curr_arg]->ToString("insn", &modifier_decode_function) << ")));\n";
										} else if (!map_name.compare("hex32")) {
											str << "append_hex((uint32_t)(" << desc->args[curr_arg]->ToString("insn", &modifier_decode_function) << "));\n";
										} else if (!map_name.compare("hex64")) {
											str << "append_hex((uint64_t)(" << desc->args[curr_arg]->ToString("insn", &modifier_decode_function) << "));\n";
										} else {
											str << "append_dec((uint32_t)(" << desc->args[curr_arg]->ToString("insn", &modifier_decode_function) << "));\n";
										}
									} else {
										// otherwise look up the map (generating an error if it is invalid) and output code to generate a map lookup
										std::map<std::string, isa::AsmMapDescription>::const_iterator map_it = isa.Mappings.find(map_name);
										if (map_it == isa.Mappings.end()) {
											// TODO: throw an exception here instead
											fprintf(stderr, "Attempted to use an undefined asm map %s for insn %s\n", map_name.c_str(), desc->instruction_name.c_str());
											return false;
										}
										str << "map_val = " << map_it->second.GetMin() << " + " << desc->args[curr_arg]->ToString("insn", modifier_decode_function) << ";"
										    "if (map_val < " << map_it->second.mapping.size() << ")"
										    "{ append_str(_map_" << isa.ISAName << "_" << map_name << "[map_val]); }"
										    "else { append_str(\"???\"); }";
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
										str << "append_str(modifier_decode_" << nameNode->getText(nameNode)->chars << "(" << desc->args[curr_arg]->ToString("insn", &modifier_decode_function) << ", insn, pc));\n";
									} else {
										// look up the map we are mapping to
										std::map<std::string, isa::AsmMapDescription>::const_iterator map_it = isa.Mappings.find(map_name);
										if (map_it == isa.Mappings.end()) {
											// error
											fprintf(stderr, "Attempted to use an undefined asm map %s for insn %s\n", map_name.c_str(), desc->instruction_name.c_str());
											return false;
										}
										isa::AsmMapDescription map = map_it->second;

										switch (mod_node->getType(mod_node)) {
											case ASM_BIT_LIST: {
												str << "uint32_t val = " << desc->args[curr_arg]->ToString("insn", &modifier_decode_function) << ";\n bool first = true;\n";
												// loop from low to high bits. a register map value represents the bit representing that register
												for (int bit = map.GetMin(); bit <= map.GetMax(); bit++) {
													uint32_t val = 1 << bit;
													str << "if(val & " << val << ")\n";
													str << "{\n";
													str << "   if(!first) append_str(\", \");\n";
													str << "append_str(_map_" << isa.ISAName << "_" << map_name << "[" << (bit - map.GetMin()) << "]);\n";
													str << "first = false;\n";
													str << "}\n";
												}
												break;
											}
											default: {
												fprintf(stderr, "Modifier is not currently supported for insn %s\n", desc->instruction_name.c_str());
												return false;
											}
										}
									}
								}
								curr_arg++;
							} else {
								str << "append_str(\"" << child->getText(child)->chars << "\");\n";
							}
						}

						parser->free(parser);
						tstream->free(tstream);
						lexer->free(lexer);
						pts->free(pts);

						str << "return;";
						if (desc->constraints.size() > 0) {
							str << "}\n";
						}
					}

					str << "}\n";

					return success;
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::DisasmGenerator, captive_disasm)
//COMPONENT_INHERITS(captive_cpu, base_decode);
