/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/GenerationManager.h"
#include "generators/DisasmGenerator.h"
#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "isa/InstructionFormatDescription.h"

#include "flexbison_harness.h"
#include "flexbison_archcasm_ast.h"
#include "flexbison_archcasm.h"

using namespace gensim::generator;

namespace gensim
{
	namespace generator
	{
		namespace captive
		{

			std::string modifier_decode_function(std::string this_str, std::string fnname, const std::vector<const util::expression *> &args)
			{
				std::stringstream output;
				output << "modifier_decode_" << fnname << "(";
				bool first = true;
				for (auto &arg : args) {
					if (!first) output << ",";
					output << arg->ToString(this_str, &modifier_decode_function);
					first = false;
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

								str << "(" << constraint->ToString("insn", &modifier_decode_function) << ")";
							}

							str << ") {";
						}

						// now, decode the format string and output

						// parse the format string
						std::stringstream input;
						input.str(desc->format);

						ArchCAsm::ArchCAsmScanner scanner(&input);
						ArchCAsm::AstNode root_node (ArchCAsmNodeType::ROOT);
						ArchCAsm::ArchCAsmParser parser(scanner, &root_node);

						// We should have already parsed the input to determine if there are syntax errors, so this parse should always succeed.
						if(parser.parse()) {
							UNEXPECTED;
						}

						// now loop through the parsed components and output code to emit them
						uint32_t curr_arg = 0;
						const auto &listNode = root_node[0];
						for (unsigned asm_arg = 0; asm_arg < listNode.GetChildren().size(); ++asm_arg) {
							auto &child = listNode[asm_arg];
							// each node is either an argument placeholder or just text
							switch(child.GetType()) {
								case ArchCAsmNodeType::Placeholder: {
									if (curr_arg >= desc->args.size()) {
										fprintf(stderr, "Too few disassembly arguments provided to disassembly for instruction %s\n", insn.Name.c_str());
										success = false;
										return success;
									}

									// if this node is an ARG, grab the identifier which names either a map or a builtin map (imm or exp)
									std::string map_name = child[0].GetString();
									const auto &modifier_list = child[1];

									// identifiers can be affected by modifiers (e.g. ... for reg lists). we can tell if a node is modified
									// by looking at how many children it has.
									if (modifier_list.GetChildren().size() == 0) { // if this arg is unmodified then process it immediately
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
										GASSERT(modifier_list.GetChildren().size() == 1);
										const auto &mod_node = modifier_list[0];

										// look up the map we are mapping to
										std::map<std::string, isa::AsmMapDescription>::const_iterator map_it = isa.Mappings.find(map_name);
										if (map_it == isa.Mappings.end()) {
											// error
											fprintf(stderr, "Attempted to use an undefined asm map %s for insn %s\n", map_name.c_str(), desc->instruction_name.c_str());
											return false;
										}
										isa::AsmMapDescription map = map_it->second;

										if(mod_node.GetString() == "...") {
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
										} else {
											UNEXPECTED;
										}

									}
									curr_arg++;
								}
								case ArchCAsmNodeType::Text: {
									str << "append_str(\"" << child[0].GetString() << "\");\n";
								}
								default: {
									UNEXPECTED;
								}
							}
						}

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
