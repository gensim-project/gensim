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

					util::cppformatstream dbt_glue_str;
					dbt_glue_str << "#pragma once\n";
					dbt_glue_str << "#include <jit/dbt.h>\n";
					dbt_glue_str << "#include <dbt/define.h>\n";
					dbt_glue_str << "#include <dbt/gen/generator.h>\n";
					dbt_glue_str << "#include <aarch64-decode.h>\n";
					dbt_glue_str << "#include <aarch64-jit3.h>\n";

					dbt_glue_str << "namespace captive {";
					dbt_glue_str << "namespace arch {";
					dbt_glue_str << "namespace " << arch.Name << " {";

					dbt_glue_str << "template<bool emit_trace_calls_>\n";
					dbt_glue_str << "class aarch64_dbt : public captive::arch::jit::DBT\n";
					dbt_glue_str << "{";
					dbt_glue_str << "public:\n";

					dbt_glue_str << "bool generate_instruction(dbt::support& support, dbt::gen::block_context& block_context, const Decode* decoded_instruction) override";
					dbt_glue_str << "{";
					dbt_glue_str << "generator_dispatcher<emit_trace_calls_> dispatcher;";
					dbt_glue_str << "return dispatcher.generate_instruction(support, block_context, *(aarch64_decode *) decoded_instruction);";
					dbt_glue_str << "}";

					dbt_glue_str << "};";

					dbt_glue_str << "}";
					dbt_glue_str << "}";
					dbt_glue_str << "}";

					WriteOutputFile(Manager.GetArch().Name + "-dbt.h", dbt_glue_str);

					util::cppformatstream header_str;
					util::cppformatstream source_str;

					// generate the source streams
					if (!GenerateJITHeader(header_str)) {
						return false;
					}

					if (!GenerateJITSource(source_str)) {
						return false;
					}

					WriteOutputFile(Manager.GetArch().Name + "-jit3.h", header_str);
					WriteOutputFile(Manager.GetArch().Name + "-jit3.cpp", source_str);

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							util::cppformatstream insn_str;

							if (!GenerateJITChunkPrologue(insn_str)) {
								return false;
							}

							if (!GenerateJITFunction(insn_str, *isa, *insn.second)) {
								return false;
							}

							bool generate_predicate_generator = isa->GetDefaultPredicated();

							if (!generate_predicate_generator && insn.second->Format->HasAttribute("predicated")) {
								generate_predicate_generator = true;
							} else if (generate_predicate_generator && insn.second->Format->HasAttribute("notpredicated")) {
								generate_predicate_generator = false;
							}

							if (generate_predicate_generator) {
								GeneratePredicateFunction(insn_str, *isa, *insn.second->Format, *insn.second);
							}

							if (!GenerateJITChunkEpilogue(insn_str)) {
								return false;
							}

							std::string filename = Manager.GetArch().Name + "-jit3-" + isa->ISAName + "-" + insn.first + ".cpp";
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
					return Manager.GetArch().Name + "_decode_" + fmt.GetISA().ISAName + "_" + fmt.GetName();
				}

				inline std::string EnumElementForInstruction(const isa::InstructionDescription& insn) const
				{
					return Manager.GetArch().Name + "_" + insn.Format->GetISA().ISAName + "_" + insn.Name;
				}

				bool GenerateJITChunkPrologue(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#include <" << arch.Name << "-jit3.h>\n";
					str << "#include <queue>\n";
					str << "#include <set>\n";

					/*const isa::ISADescription *isa = arch.ISAs.front();
					for (const auto& helper_item : isa->GetSSAContext().Actions()) {
						if (!helper_item.second->HasAttribute(ActionAttribute::Helper)) continue;

						const genc::IRHelperAction *helper = dynamic_cast<const genc::IRHelperAction*> (((const SSAFormAction *)helper_item.second)->GetAction());
						if (!helper->GetSignature().HasAttribute(ActionAttribute::NoInline) || helper->GetSignature().GetName() == "instruction_predicate") continue;

						str << "extern " << helper->GetSignature().GetType().GetCType() << " __helper_trampoline_" << helper->GetSignature().GetName() << "(" << ClassNameForCPU(true) << " *";

						for (const auto& param : helper->GetSignature().GetParams()) {
							str << ", " << param.GetType().GetCType();
						}

						str << ");";
					}*/

					str << "using namespace captive::arch::" << arch.Name << ";";
					str << "using namespace dbt::gen;";

					return true;
				}

				bool GenerateJITChunkEpilogue(util::cppformatstream& str) const
				{
					//str << "template class " << ClassNameForJIT() << "<true>;";
					//str << "template class " << ClassNameForJIT() << "<false>;";

					return true;
				}

				bool GenerateJITHeader(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#pragma once\n";

					str << "#include <define.h>\n";
					str << "#include <" << arch.Name << "-decode.h>\n";
					str << "#include <jit/builtins.h>\n";
					str << "#include <dbt/define.h>\n";
					str << "#include <dbt/gen/generator.h>\n";

					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace " << arch.Name << " {";

					for (auto isa : arch.ISAs) {
						/*for (auto fmt : isa->Formats) {
							bool generate_predicate_generator = isa->GetDefaultPredicated();

							if (!generate_predicate_generator && fmt.second->HasAttribute("predicated")) {
								generate_predicate_generator = true;
							} else if (generate_predicate_generator && fmt.second->HasAttribute("notpredicated")) {
								generate_predicate_generator = false;
							}

							if (generate_predicate_generator) {
								str << "dbt::gen gnode *generate_predicate_" << isa->ISAName << "_" << fmt.second->GetName() << "(const " << ClassNameForFormatDecoder(*fmt.second) << "& insn, dbt::gen::generator_context& context);\n";
							}
						}*/

						for (auto insn : isa->Instructions) {
							//str << "bool translate_" << isa->ISAName << "_" << insn.second->Name << "(const " << ClassNameForFormatDecoder(*insn.second->Format) << "& insn, dbt::gen::generator_context& context);";

							str << "template<bool emit_trace_calls_>\n";
							str << "class " << isa->ISAName << "_" << insn.second->Name << "_generator : public dbt::gen::generator {";
							str << "public:\n";
							str << isa->ISAName << "_" << insn.second->Name << "_generator(dbt::support& support, dbt::gen::block_context& block_context) : generator(support, block_context) { }";
							str << "bool generate(const " << ClassNameForFormatDecoder(*(insn.second->Format)) << "&);";

							bool generate_predicate_generator = isa->GetDefaultPredicated();

							if (!generate_predicate_generator && insn.second->Format->HasAttribute("predicated")) {
								generate_predicate_generator = true;
							} else if (generate_predicate_generator && insn.second->Format->HasAttribute("notpredicated")) {
								generate_predicate_generator = false;
							}

							if (generate_predicate_generator) {
								str << "private:\n";
								str << "dbt::gen::gvar *generate_predicate(const " << ClassNameForFormatDecoder(*(insn.second->Format)) << "&);";
							}

							str << "};";
						}
					}

					str << "template<bool emit_trace_calls_>\n";
					str << "class generator_dispatcher {\n";
					str << "public:\n";
					str << "bool generate_instruction(dbt::support& support, dbt::gen::block_context& block_context, " << ClassNameForDecoder() << "& instruction) {";

					str << "switch (instruction.opcode) {";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							str << "case " << ClassNameForDecoder() << "::" << EnumElementForInstruction(*insn.second) << ":\n";
							str << "{";
							//str << "return translate_" << isa->ISAName << "_" << insn.second->Name << "((const " << ClassNameForFormatDecoder(*insn.second->Format) << "&)insn, emitter);";
							str << isa->ISAName << "_" << insn.second->Name << "_generator<emit_trace_calls_> g(support, block_context);";
							str << "return g.generate((" << ClassNameForFormatDecoder(*(insn.second->Format)) << "&)instruction);";
							str << "}";
						}
					}

					str << "case " << ClassNameForDecoder() << "::" << arch.Name << "_unknown: ";
					str << "{";
					str << "dbt::gen::generator g(support, block_context);";
					str << "g.call0(&__captive_builtin_undefined_instruction, dbt::gen::gnode_type::voidtype());";
					str << "return true;";
					str << "}";
					str << "default: return false;";
					str << "}";
					str << "}";

					str << "};";

					str << "}";
					str << "}";
					str << "}";

					return true;
				}

				bool GenerateJITSource(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					str << "#include <" << arch.Name << "-jit3.h>\n";


					return true;
				}

				bool GeneratePredicateFunction(util::cppformatstream& str, const isa::ISADescription& isa, const isa::InstructionFormatDescription& fmt, const isa::InstructionDescription& insn) const
				{
					str << "template<bool emit_trace_calls_>";
					str << "gvar *" << isa.ISAName << "_" << insn.Name << "_generator<emit_trace_calls_>::generate_predicate(const " << ClassNameForFormatDecoder(fmt) << "& insn)";
					str << "{";

					str << "std::queue<block *> dynamic_block_queue;";
					str << "block *__exit_block = create_block();";
					str << "gvar *__result = create_global(gnode_type::u8type());";

					EmitJITFunction(str, *(SSAFormAction *)isa.GetSSAContext().GetAction("instruction_predicate"));

					str << "return __result;";
					str << "}";

					return true;
				}

				bool GenerateJITFunction(util::cppformatstream& src_stream, const isa::ISADescription& isa, const isa::InstructionDescription& insn) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					if (!isa.GetSSAContext().HasAction(insn.BehaviourName)) {
						fprintf(stderr, "ERROR: execute action '%s' does not exist.\n", insn.BehaviourName.c_str());
						return false;
					}

					const SSAFormAction &action = *(SSAFormAction *)isa.GetSSAContext().GetAction(insn.BehaviourName);

					src_stream << "template<bool emit_trace_calls_>";
					src_stream << "bool " << isa.ISAName << "_" << insn.Name << "_generator<emit_trace_calls_>::generate(const " << ClassNameForFormatDecoder(*insn.Format) << "& insn) {";

					FeatureUseAnalysis fua;
					auto used_features = fua.GetUsedFeatures(&action);
					for (auto feature : used_features) {
						src_stream << "get_support().use_feature(" << feature->GetId() << ");\n";
					}

					bool have_dynamic_blocks = false;
					for (const auto block : action.GetBlocks()) {
						if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
							have_dynamic_blocks = true;
							break;
						}
					}

					if (have_dynamic_blocks) {
						src_stream << "std::queue<block *> dynamic_block_queue;";
					}

					src_stream << "block *__exit_block = create_block();";

					bool generate_predicate_executor = isa.GetDefaultPredicated();

					if (!generate_predicate_executor && insn.Format->HasAttribute("predicated")) {
						generate_predicate_executor = true;
					} else if (generate_predicate_executor && insn.Format->HasAttribute("notpredicated")) {
						generate_predicate_executor = false;
					}

					if (generate_predicate_executor) {
						src_stream << "if (insn.is_predicated) {";
						src_stream << "block *__predicate_taken = create_block();";
						src_stream << "gvar *predicate_result = generate_predicate(insn);";

						src_stream << "if (emit_trace_calls_) {";
						src_stream << "block *__predicate_not_taken = create_block();";
						src_stream << "branch(load_global(predicate_result), __predicate_taken, __predicate_not_taken);\n";

						src_stream << "set_block_context(__predicate_not_taken);";
						src_stream << "trace_not_taken();";

						src_stream << "if (insn.end_of_block) {";
						src_stream << "increment_pc(constant_u8(" << (insn.Format->GetLength() / 8) << "));";
						src_stream << "}";

						src_stream << "jump(__exit_block);";

						src_stream << "} else if (insn.end_of_block) {";
						src_stream << "block *__predicate_not_taken = create_block();";
						src_stream << "branch(load_global(predicate_result), __predicate_taken, __predicate_not_taken);\n";
						src_stream << "set_block_context(__predicate_not_taken);";
						src_stream << "increment_pc(constant_u8(" << (insn.Format->GetLength() / 8) << "));";
						src_stream << "jump(__exit_block);";
						src_stream << "} else {";
						src_stream << "branch(load_global(predicate_result), __predicate_taken, __exit_block);\n";
						src_stream << "}";

						src_stream << "set_block_context(__predicate_taken);";
						src_stream << "}";
					}

					EmitJITFunction(src_stream, action);

					src_stream << "if (!insn.end_of_block) {";
					src_stream << "increment_pc(constant_u64(" << (insn.Format->GetLength() / 8) << "));";
					src_stream << "}";

					src_stream << "	return true;"
					           << "}";

					src_stream << "template class " << isa.ISAName << "_" << insn.Name << "_generator<false>;";
					src_stream << "template class " << isa.ISAName << "_" << insn.Name << "_generator<true>;";

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
						src_stream << sym->GetType().GetCType() << " CV_" << sym->GetName() << ";\n";

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
						src_stream << "if (dynamic_block_queue.size() > 0) {\n";
						src_stream << "std::set<block *> emitted_blocks;";
						src_stream << "while (dynamic_block_queue.size() > 0) {\n";

						src_stream << "block *block_index = dynamic_block_queue.front();"
						           << "dynamic_block_queue.pop();"
						           << "if (emitted_blocks.count(block_index)) continue;"
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
							src_stream << "set_block_context(block_" << block->GetName() << ");";

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

						//src_stream << "emitter.set_current_block(__exit_block);";
					} else {
						src_stream << "jump(__exit_block);\n";
					}

					src_stream << "set_block_context(__exit_block);";

					return true;
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::JITv3Generator, captive_jitv3)
