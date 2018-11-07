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

				inline std::string ClassNameForFormatDecoder(const isa::InstructionFormatDescription& fmt) const
				{
					return Manager.GetArch().Name + "_decode_" + fmt.GetISA().ISAName + "_" + fmt.GetName();
				}

				inline std::string ClassNameForISADecoder(const isa::ISADescription& isa) const
				{
					return Manager.GetArch().Name + "_decode_" + isa.ISAName;
				}

				inline std::string ClassNameForDecoder() const
				{
					return Manager.GetArch().Name + "_decode";
				}

				virtual bool GenerateDecodeHeader(util::cppformatstream &str) const override
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#pragma once\n";

					str << "#include <decode.h>\n";

					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace " << arch.Name << " {";

					// Root Arch Decode Class
					str << "class " << ClassNameForDecoder() << " : public Decode";
					str << "{";
					str << "public:\n";

					GenerateDecodeEnum(str);

					str << arch.Name << "_isa_modes isa_mode;";
					str << arch.Name << "_opcodes opcode;";
					str << "uint32_t ir;"; // XXX HACK

					str << "bool decode(uint32_t isa_mode, uint64_t insn_pc, const uint8_t *ptr) override;";
					str << "JumpInfo get_jump_info() override;";

					str << "private:\n";

					for (auto tree : decode_trees) {
						str << "bool decode_" << tree.first->ISAName << "(uint32_t ir);"; // XXX HACK
					}

					str << "};";

					// Per-ISA Decode Class
					for (auto isa : arch.ISAs) {
						str << "class " << ClassNameForISADecoder(*isa) << " : public " << ClassNameForDecoder();
						str << "{";
						str << "public:\n";
						for (auto user_field : isa->UserFields) {
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
						}
						str << "};";

						// Per-Format Decode Class
						for (auto fmt : isa->Formats) {
							str << "class " << ClassNameForFormatDecoder(*fmt.second) << " : public " << ClassNameForISADecoder(*isa);
							str << "{";
							str << "public:\n";
							auto fields = isa->Get_Decode_Fields();

							for (const auto& chunk : fmt.second->GetChunks()) {
								if (!chunk.generate_field)
									continue;

								int size = fields[chunk.Name];

								if (size < 9) {
									str << "uint8_t";
								} else if (size < 17) {
									str << "uint16_t";
								} else if (size < 33) {
									str << "uint32_t";
								} else {
									str << "??";
								}

								str << " " << chunk.Name << ";";
							}

							str << "inline void decode_behaviour()";
							str << "{";
							str << fmt.second->DecodeBehaviourCode;
							str << "}";

							bool generate_is_predicate = isa->GetDefaultPredicated();

							if (!generate_is_predicate && fmt.second->HasAttribute("predicated")) {
								generate_is_predicate = true;
							} else if (generate_is_predicate && fmt.second->HasAttribute("notpredicated")) {
								generate_is_predicate = false;
							}

							if (generate_is_predicate) {
								str << "inline bool decode_is_predicated() const";
								str << "{";
								str << *(isa->BehaviourActions.at("is_predicated"));
								str << "}";
							}

							str << "};";

							str << "static_assert(sizeof(" << ClassNameForFormatDecoder(*fmt.second) << ") <= 128, \"Decode class for format " << fmt.first << " is too big!\");";
						}
					}

					str << "}";
					str << "}";
					str << "}";

					return true;
				}

				bool GenerateDecodeLeaf(util::cppformatstream& str, const isa::ISADescription& isa, const isa::InstructionDescription& insn) const
				{
					str << "opcode = " << EnumElementForInstruction(insn) << ";";
					str << "length = " << (uint32_t) (insn.Format->GetLength() / 8) << ";";

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

					return true;
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
								str << "(ir & BIT_LSB(" << high_bit << ")) >> " << high_bit << ")\n {\n";
							else
								str << "UNSIGNED_BITS(ir, " << high_bit << "," << low_bit << ")) \n{\n";

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

					str << "#include <" << arch.Name << "-decode.h>\n";
					str << "using namespace captive::arch::" << arch.Name << ";";

					str << "bool " << ClassNameForDecoder() << "::decode(uint32_t isa_mode, uint64_t insn_pc, const uint8_t *ptr)";
					str << "{";
					str << "opcode = " << arch.Name << "_unknown;";
					str << "pc = insn_pc;";
					str << "ir = *(const uint32_t *)ptr;";
					str << "end_of_block = false;";
					str << "is_predicated = false;";

					str << "bool result = false;";
					str << "switch ((" << arch.Name << "_isa_modes)isa_mode) {";
					for (auto tree : decode_trees) {
						str << "case " << EnumElementForISA(*tree.first) << ": ";
						str << "result = decode_" << tree.first->ISAName << "(ir);";
						str << "break;";
					}
					str << "}";

					str << "if (opcode == " << arch.Name << "_unknown) {";
					str << "end_of_block = true;";
					str << "result = true;";
					str << "}";

					str << "return result;";

					str << "}";

					str << "captive::arch::JumpInfo " << ClassNameForDecoder() << "::get_jump_info()";
					str << "{";
					str << "JumpInfo info; info.type = captive::arch::JumpInfo::NONE; info.target = 0;";
					str << "switch (opcode) {";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							if (insn.second->FixedJump || insn.second->FixedJumpPred || insn.second->VariableJump) {
								str << "case " << EnumElementForInstruction(*insn.second) << ":";

								str << "info.type = ";
								if (insn.second->FixedJump) {
									str << "captive::arch::JumpInfo::DIRECT;";
									str << "info.target = pc + ((" << ClassNameForFormatDecoder(*insn.second->Format) << " *)this)->" << insn.second->FixedJumpField << ";";
								} else if (insn.second->FixedJumpPred) {
									str << "captive::arch::JumpInfo::DIRECT_PREDICATED;";
									str << "info.target = pc + ((" << ClassNameForFormatDecoder(*insn.second->Format) << " *)this)->" << insn.second->FixedJumpField << ";";
								} else {
									str << "captive::arch::JumpInfo::INDIRECT;";
								}

								str << "break;";
							}
						}
					}

					str << "case " << arch.Name << "_unknown:";
					str << "info.type = captive::arch::JumpInfo::INDIRECT;";
					str << "break;";

					str << "default: break;";
					str << "}";
					str << "return info;";
					str << "}";

					int i = 0;
					for (auto tree : decode_trees) {
						str << "inline bool " << ClassNameForDecoder() << "::decode_" << tree.first->ISAName << "(uint32_t ir)"; // XXX HACK
						str << "{";
						str << "isa_mode = " << EnumElementForISA(*tree.first) << ";";
						GenerateDecodeTree(str, *tree.first, *tree.second, i);

						str << "return false;";
						str << "}";
					}
					return true;
				}

				void GenerateDecodeEnum(util::cppformatstream& str) const
				{
					const arch::ArchDescription& arch = Manager.GetArch();

					str << "enum " << arch.Name << "_isa_modes {";
					for (auto isa : arch.ISAs) {
						str << EnumElementForISA(*isa) << " = " << isa->isa_mode_id << ",\n";
					}
					str << "};\n";

					str << "enum " << arch.Name << "_opcodes {\n";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							str << EnumElementForInstruction(*insn.second) << ",\n";
						}
					}

					str << arch.Name << "_unknown = -1\n";
					str << "};";
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::DecoderGenerator, captive_decoder)
COMPONENT_INHERITS(captive_decoder, base_decode);
