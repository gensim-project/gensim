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

#include "JITv2NodeWalker.h"
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

			class JITv2Generator : public GenerationComponent
			{
			public:

				JITv2Generator(GenerationManager &man) : GenerationComponent(man, "captive_jitv2")
				{
				}

				virtual ~JITv2Generator()
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

					bool success = true;

					util::cppformatstream header_str;
					util::cppformatstream source_str;

					// generate the source streams
					success &= GenerateJITHeader(header_str);
					success &= GenerateJITSource(source_str);

#define NR_CHUNKS 8
					util::cppformatstream insn_str[NR_CHUNKS];
					int idx = 0;

					for (int chunk = 0; chunk < NR_CHUNKS; chunk++) {
						success &= GenerateJITChunkPrologue(insn_str[chunk]);
					}

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							success &= GenerateJITFunction(insn_str[(idx++) % NR_CHUNKS], *isa, *insn.second);
						}
					}

					for (int chunk = 0; chunk < NR_CHUNKS; chunk++) {
						success &= GenerateJITChunkEpilogue(insn_str[chunk]);
					}

					if (success) {
						WriteOutputFile(Manager.GetArch().Name + "-jit2.h", header_str);
						WriteOutputFile(Manager.GetArch().Name + "-jit2.cpp", source_str);

						for (int chunk = 0; chunk < NR_CHUNKS; chunk++) {
							WriteOutputFile(Manager.GetArch().Name + "-jit2-chunk-" + std::to_string(chunk) + ".cpp", insn_str[chunk]);
						}
					}

					return success;
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
					return Manager.GetArch().Name + "_jit2";
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

					str << "#include <" << arch.Name << "-jit2.h>\n";
					str << "#include <queue>\n";
					str << "#include <set>\n";

					str << "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n";

					const isa::ISADescription *isa = arch.ISAs.front();
					for (const auto& helper_item : isa->GetSSAContext().Actions()) {
						if (!helper_item.second->HasAttribute(ActionAttribute::Helper)) continue;

						const genc::IRHelperAction *helper = dynamic_cast<const genc::IRHelperAction*> (((const SSAFormAction *)helper_item.second)->GetAction());
						if (!helper->GetSignature().HasAttribute(ActionAttribute::NoInline) || helper->GetSignature().GetName() == "instruction_predicate") continue;

						str << "extern " << helper->GetSignature().GetType().GetCType() << " __helper_trampoline_" << helper->GetSignature().GetName() << "(" << ClassNameForCPU(true) << " *";

						for (const auto& param : helper->GetSignature().GetParams()) {
							str << ", " << param.GetType().GetCType();
						}

						str << ");";
					}

					str << "using namespace captive::arch::" << arch.Name << ";";

					return true;
				}

				bool GenerateJITChunkEpilogue(util::cppformatstream& str) const
				{
					str << "template class " << ClassNameForJIT() << "<true>;";
					str << "template class " << ClassNameForJIT() << "<false>;";

					return true;
				}

				bool GenerateJITHeader(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#pragma once\n";

					str << "#include <define.h>\n";
					str << "#include <dbt/el/context.h>\n";
					str << "#include <dbt/el/emitter.h>\n";
					str << "#include <dbt/dbt.h>\n";
					str << "#include <" << arch.Name << "-decode.h>\n";

					str << "typedef uint8_t uint8;";
					str << "typedef uint16_t uint16;";
					str << "typedef uint32_t uint32;";
					str << "typedef uint64_t uint64;";
					str << "typedef int8_t sint8;";
					str << "typedef int16_t sint16;";
					str << "typedef int32_t sint32;";
					str << "typedef int64_t sint64;";

					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace " << arch.Name << " {";

					str << "class " << ClassNameForCPU() << ";";

					str << "template<bool TRACE>\n";
					str << "class " << ClassNameForJIT() << " : public captive::arch::jit::DBT {";
					str << "public:\n";
					str << ClassNameForJIT() << "();";
					str << "~" << ClassNameForJIT() << "();";
					str << "bool translate(const Decode *decode_obj, captive::arch::dbt::el::Emitter& emitter) override;";

					str << "private:\n";

					for (auto isa : arch.ISAs) {
						for (auto fmt : isa->Formats) {
							bool generate_predicate_generator = isa->GetDefaultPredicated();

							if (!generate_predicate_generator && fmt.second->HasAttribute("predicated")) {
								generate_predicate_generator = true;
							} else if (generate_predicate_generator && fmt.second->HasAttribute("notpredicated")) {
								generate_predicate_generator = false;
							}

							if (generate_predicate_generator) {
								str << "captive::arch::dbt::el::Value *generate_predicate_" << isa->ISAName << "_" << fmt.second->GetName() << "(const " << ClassNameForFormatDecoder(*fmt.second) << "& insn, captive::arch::dbt::el::Emitter& emitter);\n";
							}
						}

						for (auto insn : isa->Instructions) {
							str << "bool translate_" << isa->ISAName << "_" << insn.second->Name << "(const " << ClassNameForFormatDecoder(*insn.second->Format) << "& insn, captive::arch::dbt::el::Emitter& emitter);";
						}
					}

					str << "};";

					str << "}";
					str << "}";
					str << "}";

					return true;
				}

				bool GenerateJITSource(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();
					str << "#include <" << arch.Name << "-jit2.h>\n";
					str << "#include <" << arch.Name << "-cpu.h>\n";
					str << "#include <queue>\n";
					str << "#include <set>\n";

					str << "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n";

					const isa::ISADescription *isa = arch.ISAs.front();
					for (const auto& helper_item : isa->GetSSAContext().Actions()) {
						if (!helper_item.second->HasAttribute(ActionAttribute::Helper)) continue;

						const genc::IRHelperAction *helper = dynamic_cast<const genc::IRHelperAction*> (((const SSAFormAction *)helper_item.second)->GetAction());
						if (!helper->GetSignature().HasAttribute(ActionAttribute::NoInline) || helper->GetSignature().GetName() == "instruction_predicate") continue;

						str << helper->GetSignature().GetType().GetCType() << " __helper_trampoline_" << helper->GetSignature().GetName() << "(" << ClassNameForCPU(true) << " *cpu";

						int nr_params = 0;
						for (const auto& param : helper->GetSignature().GetParams()) {
							str << ", " << param.GetType().GetCType();
							str << " __p" << nr_params++;
						}

						str << ")\n";
						str << "{";

						if (helper->GetSignature().GetType() != gensim::genc::IRTypes::Void) str << "return ";

						str << "cpu->" << helper->GetSignature().GetName() << "(";

						for (int i = 0; i < nr_params; i++) {
							if (i > 0) {
								str << ", ";
							}

							str << "__p" << i;
						}

						str << ");";
						str << "}";
					}

					str << "void __behaviour_trampoline_undef(" << ClassNameForCPU(true) << " *cpu) { cpu->handle_undefined_instruction(); }";

					str << "using namespace captive::arch::" << arch.Name << ";";

					str << "template<bool TRACE>";
					str << ClassNameForJIT() << "<TRACE>::" << ClassNameForJIT() << "() { }";

					str << "template<bool TRACE>";
					str << ClassNameForJIT() << "<TRACE>::~" << ClassNameForJIT() << "() { }";

					str << "template<bool TRACE>";
					str << "bool " << ClassNameForJIT() << "<TRACE>::translate(const Decode *decode_obj, captive::arch::dbt::el::Emitter& emitter)";

					str << "{";
					str << "const " << ClassNameForDecoder() << "& insn = *((const " << ClassNameForDecoder() << " *)decode_obj);";
					str << "switch (insn.opcode) {";

					for (auto isa : arch.ISAs) {
						for (auto insn : isa->Instructions) {
							str << "case " << ClassNameForDecoder() << "::" << EnumElementForInstruction(*insn.second) << ":\n";
							str << "return translate_" << isa->ISAName << "_" << insn.second->Name << "((const " << ClassNameForFormatDecoder(*insn.second->Format) << "&)insn, emitter);";
						}
					}

					str << "case " << ClassNameForDecoder() << "::" << arch.Name << "_unknown: ";
					str << "#ifdef CONFIG_PRINT_UNKNOWN_INSTRUCTIONS\n";
					str << "printf(\"*** unknown instruction: %08x\\n\", insn.ir);";
					str << "#endif\n";
					str << "emitter.call((void *)&__behaviour_trampoline_undef);";
					str << "emitter.leave();";
					str << "return true;";
					str << "default: return false;";
					str << "}";
					str << "}";

					for (auto isa : arch.ISAs) {
						for (auto fmt : isa->Formats) {
							bool generate_predicate_generator = isa->GetDefaultPredicated();

							if (!generate_predicate_generator && fmt.second->HasAttribute("predicated")) {
								generate_predicate_generator = true;
							} else if (generate_predicate_generator && fmt.second->HasAttribute("notpredicated")) {
								generate_predicate_generator = false;
							}

							if (generate_predicate_generator) {
								GeneratePredicateFunction(str, *isa, *fmt.second);
							}
						}
					}

					str << "template class " << ClassNameForJIT() << "<true>;";
					str << "template class " << ClassNameForJIT() << "<false>;";

					return true;
				}

				bool GeneratePredicateFunction(util::cppformatstream& str, const isa::ISADescription& isa, const isa::InstructionFormatDescription& fmt) const
				{
					str << "template<bool TRACE>";
					str << "captive::arch::dbt::el::Value *" << ClassNameForJIT() << "<TRACE>::generate_predicate_" << isa.ISAName << "_" << fmt.GetName() << "(const " << ClassNameForFormatDecoder(fmt) << "& insn, captive::arch::dbt::el::Emitter& emitter)";
					str << "{";

					str << "std::queue<captive::arch::dbt::el::Block *> dynamic_block_queue;";
					str << "captive::arch::dbt::el::Block *__exit_block = emitter.context().create_block();";
					str << "captive::arch::dbt::el::Value *__result = emitter.alloc_local(emitter.context().types().u8(), true);";

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

					/////////
#if 0
					std::list<SSABlock *> work_queue;
					std::set<SSABlock *> seen;
					work_queue.push_back(action.EntryBlock);

					fprintf(stderr, "digraph %s {\n", insn.Name.c_str());
					while (!work_queue.empty()) {
						SSABlock *current = work_queue.front();
						work_queue.pop_front();

						if (seen.count(current)) continue;

						seen.insert(current);

						fprintf(stderr, "B_%08x", current->GetID());
						if (current == action.EntryBlock) {
							fprintf(stderr, " [color=red]");
						}
						fprintf(stderr, ";\n");

						for (auto successor : current->GetSuccessors()) {
							work_queue.push_back(successor);
							fprintf(stderr, "B_%08x -> B_%08x;\n", current->GetID(), successor->GetID());
						}
					}
					fprintf(stderr, "}\n");
#endif
					/////////

					src_stream << "template<bool TRACE>";
					src_stream << "bool " << ClassNameForJIT() << "<TRACE>::translate_" << isa.ISAName << "_" << insn.Name << "(const " << ClassNameForFormatDecoder(*insn.Format) << "&insn, captive::arch::dbt::el::Emitter& emitter)"
					           << "{";

					FeatureUseAnalysis fua;
					auto used_features = fua.GetUsedFeatures(&action);
					for (auto feature : used_features) {
						src_stream << "emitter.mark_used_feature(" << feature->GetId() << ");\n";
					}

					bool have_dynamic_blocks = false;
					for (const auto block : action.GetBlocks()) {
						if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
							have_dynamic_blocks = true;
							break;
						}
					}

					if (have_dynamic_blocks) {
						src_stream << "std::queue<captive::arch::dbt::el::Block *> dynamic_block_queue;";
					}

					src_stream << "captive::arch::dbt::el::Block *__exit_block = emitter.context().create_block();";

					bool generate_predicate_executor = isa.GetDefaultPredicated();

					if (!generate_predicate_executor && insn.Format->HasAttribute("predicated")) {
						generate_predicate_executor = true;
					} else if (generate_predicate_executor && insn.Format->HasAttribute("notpredicated")) {
						generate_predicate_executor = false;
					}

					if (generate_predicate_executor) {
						src_stream << "if (insn.is_predicated) {";
						src_stream << "auto __predicate_taken = emitter.context().create_block();";
						src_stream << "auto predicate_result = generate_predicate_" << isa.ISAName << "_" << insn.Format->GetName() << "(insn, emitter);";

						src_stream << "if (TRACE) {";
						src_stream << "auto __predicate_not_taken = emitter.context().create_block();";
						src_stream << "emitter.branch(predicate_result, __predicate_taken, __predicate_not_taken);\n";

						src_stream << "emitter.set_current_block(__predicate_not_taken);";
						src_stream << "emitter.trace(captive::arch::dbt::el::TraceEvent::NOT_TAKEN);";

						src_stream << "if (insn.end_of_block) {";
						src_stream << "emitter.inc_pc(emitter.const_u8(" << (insn.Format->GetLength() / 8) << "));";
						src_stream << "}";

						src_stream << "emitter.jump(__exit_block);";

						src_stream << "} else if (insn.end_of_block) {";
						src_stream << "auto __predicate_not_taken = emitter.context().create_block();";
						src_stream << "emitter.branch(predicate_result, __predicate_taken, __predicate_not_taken);\n";
						src_stream << "emitter.set_current_block(__predicate_not_taken);";
						src_stream << "emitter.inc_pc(emitter.const_u8(" << (insn.Format->GetLength() / 8) << "));";
						src_stream << "emitter.jump(__exit_block);";
						src_stream << "} else {";
						src_stream << "emitter.branch(predicate_result, __predicate_taken, __exit_block);\n";
						src_stream << "}";

						src_stream << "emitter.set_current_block(__predicate_taken);";
						src_stream << "}";
					}

					EmitJITFunction(src_stream, action);

					/*if (generate_predicate_executor) {
						src_stream << "emitter.set_current_block(__exit_block);";
					}*/

					src_stream << "if (!insn.end_of_block) {";
					src_stream << "emitter.inc_pc(emitter.const_u8(" << (insn.Format->GetLength() / 8) << "));";
					src_stream << "}";

					src_stream << "	return true;"
					           << "}";

					return true;
				}

				bool EmitJITFunction(util::cppformatstream& src_stream, const SSAFormAction& action) const
				{
					RegisterAllocationAnalysis raa;
					const auto& ra = raa.Allocate(&action, 6);

					JITv2NodeWalkerFactory factory(ra);

					bool have_dynamic_blocks = false;
					for (const auto block : action.GetBlocks()) {
						if (block->IsFixed() != BLOCK_ALWAYS_CONST) {
							have_dynamic_blocks = true;
							src_stream << "auto block_" << block->GetName() << " = emitter.context().create_block();";
						}
					}

					for (const auto sym : action.Symbols()) {
						if(sym->GetType().Reference || sym->SType == Symbol_Parameter) {
							continue;
						}

						bool symbol_has_fixed_uses = false;
						bool symbol_has_dynamic_uses = false;

						SSAValue *sym_val = (SSAValue *)sym;
						for (const auto use : sym_val->GetUses()) {
							if (auto use_stmt = dynamic_cast<SSAStatement *>(use)) {
								if (use_stmt->IsFixed()) {
									symbol_has_fixed_uses = true;
								} else {
									symbol_has_dynamic_uses = true;
								}
							}
						}

						if (symbol_has_fixed_uses) {
							src_stream << sym->GetType().GetCType() << " CV_" << sym->GetName() << ";\n";
						}

						if (symbol_has_dynamic_uses) {
							src_stream << "auto DV_" << sym->GetName() << " = emitter.alloc_local(emitter.context().types()." << sym->GetType().GetUnifiedType() << "(), ";

							bool is_global = sym->HasDynamicUses();

							src_stream << (is_global ? "true" : "false");
							src_stream << ");\n";
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
						//src_stream << "if (dynamic_block_queue.size() == 0) {\n";
						//src_stream << "emitter.jump(__exit_block);\n";
						//src_stream << "} else {";

						src_stream << "if (dynamic_block_queue.size() > 0) {\n";
						src_stream << "std::set<captive::arch::dbt::el::Block *> emitted_blocks;";
						src_stream << "while (dynamic_block_queue.size() > 0) {\n";

						src_stream << "captive::arch::dbt::el::Block *block_index = dynamic_block_queue.front();"
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
							src_stream << "emitter.set_current_block(block_" << block->GetName() << ");";

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
						src_stream << "emitter.jump(__exit_block);\n";
						src_stream << "}";

						//src_stream << "emitter.set_current_block(__exit_block);";
					} else {
						src_stream << "emitter.jump(__exit_block);\n";
					}

					src_stream << "emitter.set_current_block(__exit_block);";

					return true;
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::JITv2Generator, captive_jitv2)
