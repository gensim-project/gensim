/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "generators/DecodeGenerator.h"
#include "DecodeTree.h"

using namespace gensim::generator;

namespace gensim
{
	namespace generator
	{
		namespace captive
		{

			class DecoderGenerator : public DecodeGenerator
			{
			public:

				DecoderGenerator(GenerationManager &man) : DecodeGenerator(man, "captive_decoder")
				{
				}

				virtual ~DecoderGenerator()
				{
				}

			protected:

				const std::string GetDecodeHeaderFilename() const override
				{
					return Manager.GetArch().Name + "-decode.h";
				}

				const std::string GetDecodeSourceFilename() const override
				{
					return Manager.GetArch().Name + "-decode.cpp";
				}

			private:

				inline std::string EnumElementForInstruction(const isa::InstructionDescription& insn) const
				{
					return Manager.GetArch().Name + "_" + insn.Format->GetISA().ISAName + "_" + insn.Name;
				}

				inline std::string EnumElementForISA(const isa::ISADescription& isa) const
				{
					return Manager.GetArch().Name + "_" + isa.ISAName;
				}

				inline std::string ClassNameForInstructionDecoder(const isa::InstructionDescription& insn) const
				{
					return Manager.GetArch().Name + "_" + insn.Format->GetISA().ISAName + "_" + insn.Name + "_instruction";
				}

				inline std::string ClassNameForFormatDecoder(const isa::InstructionFormatDescription& fmt) const
				{
					return Manager.GetArch().Name + "_" + fmt.GetISA().ISAName + "_" + fmt.GetName() + "_format";
				}

				inline std::string ClassNameForISADecoder(const isa::ISADescription& isa) const
				{
					return Manager.GetArch().Name + "_" + isa.ISAName + "_isa";
				}

				inline std::string ClassNameForBaseInstruction() const
				{
					return Manager.GetArch().Name + "_instruction";
				}

				virtual bool GenerateDecodeHeader(util::cppformatstream &str) const override
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#pragma once\n";

					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace guest {";
					str << "namespace " << arch.Name << " {";
					str << "namespace isa {";

					// OPCODE Enum
					GenerateDecodeEnum(str);

					// Root Arch Decode Class
					str << "struct " << ClassNameForBaseInstruction();
					str << "{";
					str << "u32 length() const { return 4; }";
					str << "};";

					// Per-ISA Decode Class
					for (auto isa : arch.ISAs) {
						str << "struct " << ClassNameForISADecoder(*isa) << " : public " << ClassNameForBaseInstruction();
						str << "{";
						/*for (auto user_field : isa->UserFields) {
							if (user_field.field_type == "uint64") {
								str << "uint64_t";
							} else if (user_field.field_type == "uint32") {
								str << "uint32_t";
							} else if (user_field.field_type == "uint16") {
								str << "uint16_t";
							} else if (user_field.field_type == "uint8") {
								str << "uint8_t";
							} else if (user_field.field_type == "sint64") {
								str << "int64_t";
							} else if (user_field.field_type == "sint32") {
								str << "int32_t";
							} else if (user_field.field_type == "sint16") {
								str << "int16_t";
							} else if (user_field.field_type == "sint8") {
								str << "int8_t";
							} else {
								str << "??";
							}

							str << " " << user_field.field_name << ";";
						}*/

						str << arch.Name << "_" << isa->ISAName <<  "_opcodes opcode() const;";
						str << "};";

						// Per-Format Decode Class
						for (auto fmt : isa->Formats) {
							str << "struct " << ClassNameForFormatDecoder(*fmt.second) << " : public " << ClassNameForISADecoder(*isa);
							str << "{";

							int index = 0;
							for (const auto& chunk : fmt.second->GetChunks()) {
								if (chunk.is_constrained) {
									str << "u32 __fixed_val_" << index++ << " /* == " << chunk.constrained_value << " */ : " << (unsigned int)chunk.length << ";";
								} else {
									str << "u32 " << chunk.Name << " : " << (unsigned int)chunk.length << ";";
								}
							}

							str << "} __packed;";

							str << "static_assert(sizeof(" << ClassNameForFormatDecoder(*fmt.second) << ") == 4, \"Instruction format " << fmt.first << " is incorrectly sized!\");";

							for (auto insn : fmt.second->GetInstructions()) {
								str << "struct " << ClassNameForInstructionDecoder(*insn) << " : public " << ClassNameForFormatDecoder(*fmt.second);
								str << "{";

								if (insn->EOB_Contraints.size() == 0) {
									str << "bool end_of_block() const { return false; }";
								} else {
									str << "bool end_of_block() const {";
									for (const auto& eob : insn->EOB_Contraints) {
										if (eob.size() == 0) {
											str << "return true;";
											break;
										}

										str << "if (";

										bool first = true;
										for (const auto& con : eob) {
											if (!first) {
												str << " && ";
											} else {
												first = false;
											}

											switch (con.Type) {
												case isa::InstructionDescription::Constraint_Equals:
													str << "(" << "((" << ClassNameForFormatDecoder(*insn->Format) << "&)*this)." << con.Field << " == " << con.Value << ")";
													break;
												case isa::InstructionDescription::Constraint_BitwiseAnd:
													str << "((" << "((" << ClassNameForFormatDecoder(*insn->Format) << "&)*this)." << con.Field << " & " << con.Value << ") == " << con.Value << ")";
													break;
												default:
													str << "(" << "((" << ClassNameForFormatDecoder(*insn->Format) << "&)*this)." << con.Field << " != " << con.Value << ")";
													break;
											}
										}

										str << ") { return true; } else { return false; }";
									}
									str << "}";
								}

								str << "};";
							}
						}
					}

					str << "}";
					str << "}";
					str << "}";
					str << "}";
					str << "}";

					return true;
				}

				bool GenerateDecodeLeaf(util::cppformatstream& str, const isa::ISADescription& isa, const isa::InstructionDescription& insn) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "return " << arch.Name << "_" << isa.ISAName << "_opcodes::" << insn.Name << ";";
					return true;

					/*str << "length = " << (uint32_t) (insn.Format->GetLength() / 8) << ";";

					int pos = 31;

					for (const auto& chunk : insn.Format->GetChunks()) {
						if (chunk.is_constrained) {
							pos -= chunk.length;
							continue;
						}

						if (!chunk.generate_field) {
							pos -= chunk.length;
							continue;
						}

						if (isa.IsFieldOrthogonal(chunk.Name)) {
							pos -= chunk.length;
							continue;
						}

						assert(pos >= 0);
						assert(pos - chunk.length + 1 >= 0);

						str << "((" << ClassNameForFormatDecoder(*insn.Format) << "&)*this).";

						if (chunk.length == 1) {
							str << chunk.Name << " = BITSEL(ir, " << pos << ");\n";
						} else {
							str << chunk.Name << " = " << "UNSIGNED_BITS(ir, " << pos << "," << pos - chunk.length + 1 << ");\n";
						}

						pos -= chunk.length;
					}

					str << "((" << ClassNameForFormatDecoder(*insn.Format) << "&)*this).decode_behaviour();";

					for (const auto& eob : insn.EOB_Contraints) {
						if (eob.size() == 0) {
							str << "end_of_block = true;";
							break;
						}

						str << "if (";

						bool first = true;
						for (const auto& con : eob) {
							if (!first) {
								str << " && ";
							} else {
								first = false;
							}

							switch (con.Type) {
								case isa::InstructionDescription::Constraint_Equals:
									str << "(" << "((" << ClassNameForFormatDecoder(*insn.Format) << "&)*this)." << con.Field << " == " << con.Value << ")";
									break;
								case isa::InstructionDescription::Constraint_BitwiseAnd:
									str << "((" << "((" << ClassNameForFormatDecoder(*insn.Format) << "&)*this)." << con.Field << " & " << con.Value << ") == " << con.Value << ")";
									break;
								default:
									str << "(" << "((" << ClassNameForFormatDecoder(*insn.Format) << "&)*this)." << con.Field << " != " << con.Value << ")";
									break;
							}
						}

						str << ") end_of_block = true;";
					}

					bool generate_is_predicate = isa.GetDefaultPredicated();

					if (!generate_is_predicate && insn.Format->HasAttribute("predicated")) {
						generate_is_predicate = true;
					} else if (generate_is_predicate && insn.Format->HasAttribute("notpredicated")) {
						generate_is_predicate = false;
					}

					if (!generate_is_predicate)
						str << "is_predicated = false;";
					else
						str << "is_predicated = ((" << ClassNameForFormatDecoder(*insn.Format) << "&)*this).decode_is_predicated();";

					str << "return true;";

					return true;*/
				}

				bool GenerateDecodeTree(util::cppformatstream& str, const isa::ISADescription& isa, const DecodeNode& node, int& i) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					bool success = true;

					// have we seen any transitions from this node? used to emit 'else' keywords
					bool seen_transitions = false;

					str << "// Node " << node.node_number << "\n";

					if (GetProperty("Debug") == "1") str << "printf(\"Node %u\\n\"," << node.node_number << ");\n";

					// if this is a leaf node
					if (node.target) {
						GenerateDecodeLeaf(str, isa, *node.target);
					} else {
						// otherwise look at the transitions from this node and sort them by their length
						std::map<uint8_t, std::list<DecodeTransition> > sorted_transitions;
						for (auto trans : node.transitions) {
							if (sorted_transitions.find(trans.first) == sorted_transitions.end()) {
								sorted_transitions.insert(std::pair<uint8_t, std::list<DecodeTransition> >(trans.first, std::list<DecodeTransition>()));
							}

							sorted_transitions.at(trans.first).push_back(trans.second);
						}

						// loop over groups in reverse order to get longest length first - essentially we implement longest-prefix-matching
						for (std::map<uint8_t, std::list<DecodeTransition> >::reverse_iterator group = sorted_transitions.rbegin(); group != sorted_transitions.rend(); ++group) {
							std::list<DecodeTransition> &list = group->second;

							// figure out which bits of the instruction code we should be looking at
							uint32_t low_bit = (isa.GetMaxInstructionLength() - node.start_ptr - group->first);
							uint32_t high_bit = (isa.GetMaxInstructionLength() - 1 - node.start_ptr);

							str << "switch (";

							// if this transition is only one bit long, use a faster way of getting at that bit
							if (group->first == 1)
								str << "(instruction_data & BIT_LSB(" << high_bit << ")) >> " << high_bit << ")\n {\n";
							else
								str << "UNSIGNED_BITS(instruction_data, " << high_bit << "," << low_bit << ")) \n{\n";

							// loop through the transitions of this group and emit their subtrees
							for (std::list<DecodeTransition>::iterator trans = list.begin(); trans != list.end(); ++trans) {
								str << "case " << (trans->value) << ": { \n";

								success &= GenerateDecodeTree(str, isa, *(trans->target), i);

								str << "\nbreak; }\n";
							}
							str << "}\n";
						}

						// if we have an unconstrained transition (i.e., we're allowed to match 'anything')
						if (node.unconstrained_transition) {
							if (seen_transitions) str << "{\n";
							// emit an 'anything' subtree
							success &= GenerateDecodeTree(str, isa, *(node.unconstrained_transition->target), i);
							if (seen_transitions) str << "}\n";
							seen_transitions = true;
						}
					}

					return true;
				}

				virtual bool GenerateDecodeSource(util::cppformatstream &str) const override
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#include <captive/arch/guest/" << arch.Name << "/isa/" << arch.Name << "-decode.h>\n";

					// Helpers

					str << "#define UNSIGNED_BITS(v, u, l) (((u32)(v) << (31 - u)) >> (31 - u + l))\n";
					str << "#define SIGNED_BITS(v, u, l) (((s32)(v) << (31 - u)) >> (31 - u + l))\n";
					str << "#define BITSEL(v, b) (((v) >> b) & 1UL)\n";
					str << "#define BIT_LSB(i) (1 << (i))\n";

					/*#define __ROR(v, s, a) (((v) >> a) | ((v) << (s - a)))
					#define __ROR32(v, a) __ROR((uint32_t)(v), 32, a)
					#define __ROR64(v, a) __ROR((uint64_t)(v), 64, a)
					#define __SEXT64(v, from) (((int64_t)(((uint64_t)(v)) << (64 - (from)))) >> (64 - (from)))

					#define __ONES(a) (~0ULL >> (64 - (a)))
					#define __CLZ32(a) __builtin_clz((a))*/

					str << "using namespace captive::arch::guest::" << arch.Name << "::isa;";

					int i = 0;
					for (auto tree : decode_trees) {
						str << arch.Name << "_" << tree.first->ISAName << "_opcodes " << arch.Name << "_" << tree.first->ISAName << "_instruction::opcode() const";
						str << "{";
						str << "u32 instruction_data = *(u32 *)this;";
						GenerateDecodeTree(str, *tree.first, *tree.second, i);
						str << "return " << arch.Name << "_" << tree.first->ISAName << "_opcodes::__unknown__;";
						str << "}";
					}

					return true;
				}

				void GenerateDecodeEnum(util::cppformatstream& str) const
				{
					const arch::ArchDescription& arch = Manager.GetArch();

					str << "enum class " << arch.Name << "_isas {";
					for (auto isa : arch.ISAs) {
						str << EnumElementForISA(*isa) << " = " << isa->isa_mode_id << ",\n";
					}
					str << "};\n";


					for (auto isa : arch.ISAs) {
						str << "enum class " << arch.Name << "_" << isa->ISAName <<  "_opcodes {\n";

						for (auto insn : isa->Instructions) {
							str << insn.second->Name << ",\n";
						}
					}

					str << "__unknown__ = -1\n";
					str << "};";
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::DecoderGenerator, captive_decoder)
COMPONENT_INHERITS(captive_decoder, base_decode);
