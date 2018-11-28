/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/GenerationManager.h"
#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "isa/InstructionFormatDescription.h"

#include "genC/ir/IRAction.h"

#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/analysis/RegisterAllocationAnalysis.h"
#include "genC/ssa/analysis/FeatureUseAnalysis.h"
#include "genC/ssa/analysis/CallGraphAnalysis.h"

#include "JITv3NodeWalker.h"
#include "genC/ssa/statement/SSAVariableReadStatement.h"
#include "genC/ssa/statement/SSAVariableKillStatement.h"

using namespace gensim::generator;
using namespace gensim::genc::ssa;
using namespace gensim::genc;

namespace gensim
{
	namespace generator
	{
		namespace captive
		{

			class JITv3Generator : public GenerationComponent
			{
			public:

				JITv3Generator(GenerationManager &man) : GenerationComponent(man, "captive_jitv3")
				{
				}

				virtual ~JITv3Generator()
				{
				}

				std::string GetFunction() const override
				{
					return "jit";
				}

				const std::vector<std::string> GetSources() const override
				{
					return std::vector<std::string>();
				}

				bool Generate() const override
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					util::cppformatstream header_str;
					util::cppformatstream source_str;

					// generate the source streams
					if (!GenerateJITHeader(header_str)) {
						return false;
					}

					if (!GenerateJITSource(source_str)) {
						return false;
					}

					WriteOutputFile(Manager.GetArch().Name + "-generator.h", header_str);
					WriteOutputFile(Manager.GetArch().Name + "-generator.cpp", source_str);

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							util::cppformatstream insn_str;

							if (!GenerateJITChunkPrologue(insn_str)) {
								return false;
							}

							if (!GenerateJITFunction(insn_str, *isa, *insn.second)) {
								return false;
							}

							if (!GenerateJITChunkEpilogue(insn_str)) {
								return false;
							}

							std::string filename = Manager.GetArch().Name + "-generator-" + isa->ISAName + "-" + insn.first + ".cpp";
							WriteOutputFile(filename, insn_str);
						}
					}

					return true;
				}

			private:

				inline std::string ClassNameForCPU(bool qualify = false) const
				{
					if (qualify)
						return "captive::arch::" + Manager.GetArch().Name + "::" + Manager.GetArch().Name + "_cpu";
					else
						return Manager.GetArch().Name + "_cpu";
				}

				inline std::string ClassNameForJIT() const
				{
					return Manager.GetArch().Name + "_jit3";
				}

				inline std::string ClassNameForDecoder() const
				{
					return Manager.GetArch().Name + "_decode";
				}

				inline std::string ClassNameForFormatDecoder(const isa::InstructionFormatDescription& fmt) const
				{
					return "captive::arch::guest::" + Manager.GetArch().Name + "::isa::" + Manager.GetArch().Name + "_" + fmt.GetISA().ISAName + "_" + fmt.GetName() + "_format";
				}

				inline std::string ClassNameForInstructionDecoder(const isa::InstructionDescription& insn) const
				{
					return "captive::arch::guest::" + Manager.GetArch().Name + "::isa::" + Manager.GetArch().Name + "_" + insn.Format->GetISA().ISAName + "_" + insn.Name + "_instruction";
				}

				inline std::string EnumElementForInstruction(const isa::InstructionDescription& insn) const
				{
					return Manager.GetArch().Name + "_" + insn.Format->GetISA().ISAName + "_" + insn.Name;
				}

				bool GenerateJITChunkPrologue(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#include <captive/arch/guest/" << arch.Name << "/dbt/" << arch.Name << "-generator.h>\n";
					str << "#include <captive/util/list.h>\n";
					str << "#include <captive/util/set.h>\n";

					str << "using namespace captive::arch::guest::" << arch.Name << "::dbt;";
					str << "using namespace captive::crt::ee::dbt;";
					str << "using namespace captive::util;";

					return true;
				}

				bool GenerateJITChunkEpilogue(util::cppformatstream& str) const
				{
					return true;
				}

				bool GenerateJITHeader(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					const std::string& arch_name = arch.Name;

					str << "#pragma once\n";
					str << "#include <captive/crt/ee/dbt/generator.h>\n";
					str << "#include <captive/arch/guest/"<<arch_name<<"/isa/"<<arch_name<<"-decode.h>\n";
					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace guest {";
					str << "namespace "<<arch_name<<" {";
					str << "namespace dbt {";
					str << "	template<bool generate_trace_>\n";
					str << "	class "<<arch_name<<"_generator : public crt::ee::dbt::generator {";
					str << "	public:\n";
					str << "		crt::ee::dbt::generation_result generate(crt::ee::dbt::compilation_context& c, arch::guest::address_space& as, off_t& offset) override;";

					str << "private:\n";
					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							str << "crt::ee::dbt::generation_result generate_" << isa->ISAName << "_" << insn.first << "(crt::ee::dbt::compilation_context&, const " << ClassNameForInstructionDecoder(*insn.second) << "&);";
						}
					}

					str << "	};";
					str << "}";
					str << "}";
					str << "}";
					str << "}";
					str << "}";

					return true;
				}

				bool GenerateJITSource(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					const std::string& arch_name = arch.Name;

					str << "#include <captive/arch/guest/" << arch_name << "/dbt/" << arch_name << "-generator.h>\n";

					str << "#include <captive/arch/guest/"<<arch_name<<"/isa/"<<arch_name<<"-decode.h>\n";
					str << "#include <captive/arch/guest/address-space.h>\n";
					str << "#include <captive/arch/guest/address-space-region.h>\n";

					str << "using namespace captive::crt::ee::dbt;";
					str << "using namespace captive::arch::guest::"<<arch_name<<"::dbt;";
					str << "using namespace captive::arch::guest::"<<arch_name<<"::isa;";

					str << "template<bool generate_trace_>";
					str << "generation_result "<<arch_name<<"_generator<generate_trace_>::generate(compilation_context& c, arch::guest::address_space& as, off_t& offset)";
					str << "{";
					str << "	char buffer[4];";
					str << "	as.read(buffer, offset, sizeof(buffer));";

					str << "	const "<<arch_name<<"_a64_isa *i = (const "<<arch_name<<"_a64_isa *)buffer;";
					str << "	offset += i->length();";

					str << "	"<<arch_name<<"_a64_opcodes opcode = i->opcode();";

					str << "	switch (opcode) {";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							//str << "crt::ee::dbt::generation_result generate_" << isa->ISAName << "_" << insn.first << "(crt::ee::dbt::compilation_context&, const " << ClassNameForFormatDecoder(*(insn.second->Format)) << "&);";
							str << "case "<<arch_name<<"_a64_opcodes::" << insn.first << ":\n";
							str << "return generate_" << isa->ISAName << "_" << insn.first << "(c, *(const " << ClassNameForInstructionDecoder(*insn.second) << " *)i);";
						}
					}

					str << "case "<<arch_name<<"_a64_opcodes::__unknown__:\n";
					str << "		return generation_result::abort;";

					str << "	default:\n";
					str << "		return generation_result::abort;";
					str << "	}";
					str << "}";

					str << "template class "<<arch_name<<"_generator<true>;";
					str << "template class "<<arch_name<<"_generator<false>;";

					return true;
				}

				bool GenerateJITFunction(util::cppformatstream& src_stream, const isa::ISADescription& isa, const isa::InstructionDescription& insn) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					const std::string& arch_name = arch.Name;

					if (!isa.GetSSAContext().HasAction(insn.BehaviourName)) {
						fprintf(stderr, "ERROR: execute action '%s' does not exist.\n", insn.BehaviourName.c_str());
						return false;
					}

					const SSAFormAction &action = *(SSAFormAction *)isa.GetSSAContext().GetAction(insn.BehaviourName);

					src_stream << "template<bool emit_trace_calls_>";
					src_stream << "generation_result "<<arch_name<<"_generator<emit_trace_calls_>::generate_" << isa.ISAName << "_" << insn.Name << "(compilation_context& c, const " << ClassNameForInstructionDecoder(insn) << "& insn) {";

					FeatureUseAnalysis fua;
					auto used_features = fua.GetUsedFeatures(&action);
					for (auto feature : used_features) {
						src_stream << "c.use_feature(" << feature->GetId() << ");\n";
					}

					bool have_dynamic_blocks = false;
					for (const auto block : action.GetBlocks()) {
						if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
							have_dynamic_blocks = true;
							break;
						}
					}

					if (have_dynamic_blocks) {
						src_stream << "list<block *> dynamic_block_queue;";
						src_stream << "block *__exit_block = create_block();";
					}

					EmitJITFunction(src_stream, action);

					src_stream << "if (!insn.end_of_block()) {";
					src_stream << "increment_pc(constant_u64(" << (insn.Format->GetLength() / 8) << "));";
					src_stream << "return generation_result::continue_generation;";
					src_stream << "} else {";
					src_stream << "return generation_result::finalise;";
					src_stream << "}";

					src_stream << "}";

					src_stream << "template generation_result "<<arch_name<<"_generator<true>::generate_" << isa.ISAName << "_" << insn.Name << "(compilation_context&, const " << ClassNameForInstructionDecoder(insn) << "&);";
					src_stream << "template generation_result "<<arch_name<<"_generator<false>::generate_" << isa.ISAName << "_" << insn.Name << "(compilation_context&, const " << ClassNameForInstructionDecoder(insn) << "&);";

					return true;
				}

				bool EmitJITFunction(util::cppformatstream& src_stream, const SSAFormAction& action) const
				{
					JITv3NodeWalkerFactory factory;

					bool have_dynamic_blocks = false;
					for (const auto block : action.GetBlocks()) {
						if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
							have_dynamic_blocks = true;
							src_stream << "block *block_" << block->GetName() << " = create_block();";
						}
					}

					for (const auto sym : action.Symbols()) {
						if(sym->GetType().Reference || sym->SType == Symbol_Parameter) {
							continue;
						}
						src_stream << sym->GetType().GetUnifiedType() << " CV_" << sym->GetName() << ";\n";

						bool is_global = sym->HasDynamicUses();

						if (is_global) {
							src_stream << "auto DV_" << sym->GetName() << " = create_global(gnode_type::" << sym->GetType().GetUnifiedType() << "type());";
						} else {
							src_stream << "auto DV_" << sym->GetName() << " = create_local(gnode_type::" << sym->GetType().GetUnifiedType() << "type());";
						}
					}

					src_stream << "goto fixed_block_" << action.EntryBlock->GetName() << ";\n";
					for (const auto block : action.GetBlocks()) {
						if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
							src_stream << "// BLOCK " << block->GetName() << " not fully fixed\n";
							continue;
						}

						src_stream << "/* ";
						for (const auto pred : block->GetPredecessors()) {
							src_stream << pred->GetName() << ", ";
						}

						src_stream << " */\n";
						src_stream << "fixed_block_" << block->GetName() << ": {\n";

						for (const auto stmt : block->GetStatements()) {
							auto walker = factory.Create(stmt);

							src_stream << "/* " << stmt->GetDiag() << " ";
							if (stmt->IsFixed())
								src_stream << "[F] ";
							else
								src_stream << "[D] ";
							src_stream << stmt->ToString() << " */\n";

							if (stmt->IsFixed())
								walker->EmitFixedCode(src_stream, "fixed_done", true);
							else
								walker->EmitDynamicCode(src_stream, "fixed_done", true);
						}

						src_stream << "}\n";

					}

					src_stream << "fixed_done:\n";

					if (have_dynamic_blocks) {
						src_stream << "if (dynamic_block_queue.count() > 0) {\n";
						src_stream << "set<block *> emitted_blocks;";
						src_stream << "while (dynamic_block_queue.count() > 0) {\n";

						src_stream << "block *block_index = dynamic_block_queue.dequeue();"
						           << "if (emitted_blocks.contains(block_index)) continue;"
						           << "emitted_blocks.insert(block_index);";

						bool first = true;
						for (const auto block : action.GetBlocks()) {
							if (block->IsFixed() == BLOCK_ALWAYS_CONST) continue;

							if (first) {
								first = false;
							} else {
								src_stream << "else ";
							}

							src_stream << "if (block_index == block_" << block->GetName() << ") // BLOCK START LINE " << block->GetStartLine() << ", END LINE " << block->GetEndLine() << "\n";
							src_stream << "{";
							src_stream << "set_current_block(block_" << block->GetName() << ");";

							for (const auto stmt : block->GetStatements()) {
								src_stream << "/* " << stmt->GetDiag() << " ";
								if (stmt->IsFixed())
									src_stream << "[F] ";
								else
									src_stream << "[D] ";
								src_stream << stmt->ToString() << " */\n";

								if (stmt->IsFixed())
									factory.GetOrCreate(stmt)->EmitFixedCode(src_stream, "", false);
								else
									factory.GetOrCreate(stmt)->EmitDynamicCode(src_stream, "", false);
							}

							src_stream << "}";
						}

						src_stream << "}\n"; // WHILE
						src_stream << "} else {"; // ELSE
						src_stream << "jump(__exit_block);\n";
						src_stream << "}";

						src_stream << "set_current_block(__exit_block);";
					} else {
						//src_stream << "jump(__exit_block);\n";
					}

					//src_stream << "set_current_block(__exit_block);";

					return true;
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::JITv3Generator, captive_jitv3)
