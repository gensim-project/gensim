/*
 * GenCInterpreter.cpp
 *
 *  Created on: 10 Dec 2013
 *      Author: harry
 */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/GenCInterpreter/GenCInterpreterGenerator.h"
#include "generators/GenCInterpreter/InterpreterNodeWalker.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/Parser.h"
#include "genC/ir/IRAction.h"

DEFINE_COMPONENT(gensim::generator::GenCInterpreterGenerator, genc_interpret)
COMPONENT_INHERITS(genc_interpret, interpret)

namespace gensim
{

	namespace generator
	{

		GenCInterpreterGenerator::GenCInterpreterGenerator(const GenerationManager &man) : InterpretiveExecutionEngineGenerator(man, "genc_interpret") {}

		bool GenCInterpreterGenerator::GenerateExecutionForBehaviour(util::cppformatstream &str, bool trace_enabled, std::string behaviourname, const isa::ISADescription &isa) const
		{
			str << "execute_" << isa.ISAName << "_" << behaviourname << (trace_enabled ? "_trace" : "") << "(inst);";
			return true;
		}

		bool GenCInterpreterGenerator::GeneratePrototype(util::cppformatstream &stream, const gensim::isa::ISADescription &isa, const genc::ssa::SSAFormAction &action) const
		{
			stream << "template<bool trace=false> " << action.GetPrototype().ReturnType().GetCType() << " helper_" << isa.ISAName << "_" << action.GetPrototype().GetIRSignature().GetName() << "(archsim::core::execution::ExecutionEngine *ee, archsim::core::thread::ThreadInstance *thread";

			for(auto i : action.ParamSymbols) {
				// if we're accessing a struct, assume that it's an instruction
				if(i->GetType().IsStruct()) {
					stream << ", gensim::" << Manager.GetArch().Name << "::Decode &inst";
				} else {
					auto type_string = i->GetType().GetCType();
					stream << ", " << type_string << " " << i->GetName();
				}
			}
			stream << ")";
		}
		
		bool GenCInterpreterGenerator::GenerateExecuteBodyFor(util::cppformatstream &str, const genc::ssa::SSAFormAction &action) const
		{
			using namespace genc::ssa;

			str << "{";
			gensim::genc::IRType rtype = action.GetPrototype().ReturnType();
			if(rtype != gensim::genc::IRTypes::Void) {
				str << rtype.GetCType() << " _rval_;";
			}

			// first of all, emit slots for each variable we will be using
			for (auto *sym : action.Symbols()) {
				if(sym->SType == genc::Symbol_Parameter) continue;
				if (sym->IsReference()) continue;
				str << sym->GetType().GetCType() << " " << sym->GetName() << ";";
			}

			// emit a jump to the entry block (since it might not be the first one)
			str << "goto __block_" << action.EntryBlock->GetName() << ";";

			// now iterate over the blocks in the action and emit them
			for (SSAFormAction::BlockListConstIterator block_ci = action.Blocks.begin(); block_ci != action.Blocks.end(); ++block_ci) {
				GenCInterpreterNodeFactory fact;
				const SSABlock &block = **block_ci;

				str << "{";
				str << "__block_" << block.GetName() << ": (void)0;";
				//				str << "puts(\"C B " << block.GetName() << " " << block.GetStatements().front()->GetISA() << " " << block.GetStartLine() << "\");";
				for (SSABlock::StatementListConstIterator stmt_ci = block.GetStatements().begin(); stmt_ci != block.GetStatements().end(); ++stmt_ci) {
					const SSAStatement &stmt = **stmt_ci;
//			str << "//" << stmt.PrettyPrint() << "\n";

					str << "/* " << stmt.GetDiag() << " ";
					if (stmt.IsFixed())
						str << "[F] ";
					else
						str << "[D] ";
					str << stmt.ToString() << " */\n";

					fact.GetOrCreate(&stmt)->EmitFixedCode(str, "__end", true);
				}
				str << "}";
			}

			str << "__end:;";
			if(action.GetPrototype().GetIRSignature().GetType() != genc::IRTypes::Void) {
				str << "return _rval_;";
			}
			str << "}";

			return true;
		}

		bool GenCInterpreterGenerator::GenerateExtraProcessorIncludes(util::cppformatstream &str) const
		{
			str << "#include \"translate/jit_funs.h\"\n";
			str << "#include \"util/Vector.h\"\n";
			str << "#include <math.h>\n";
			str << "#include <cfenv>\n";

			return true;
		}

		bool GenCInterpreterGenerator::GenerateExtraProcessorClassMembers(util::cppformatstream &str) const
		{
			const arch::ArchDescription::ISAListType isalist = Manager.GetArch().ISAs;
			for (auto isa_ci : isalist) {
				const isa::ISADescription &isa = *isa_ci;
				for (auto behaviour_i : isa.ExecuteActions) {
					std::string behaviour_name = behaviour_i.first;
					str << "void execute_" << isa.ISAName << "_" << behaviour_name << "(const Decode &inst);";
					str << "void execute_" << isa.ISAName << "_" << behaviour_name << "_trace(const Decode &inst);";
				}
			}

			return true;
		}

		bool GenCInterpreterGenerator::GenerateExtraProcessorInitSource(util::cppformatstream &str) const
		{
			return true;
		}

		bool GenCInterpreterGenerator::GenerateExtraProcessorDestructorSource(util::cppformatstream &str) const
		{
			return true;
		}

		bool GenCInterpreterGenerator::GenerateInlineHelperFns(util::cppformatstream &str) const
		{
			// TODO: Review this multi-isa stuff - because it's semi wrong.
			isa::ISADescription *isa = Manager.GetArch().ISAs.front();

			for (const auto &action_item : isa->GetSSAContext().Actions()) {
				if (!action_item.second->HasAttribute(gensim::genc::ActionAttribute::Helper)) continue;

				auto action = dynamic_cast<genc::ssa::SSAFormAction *>(action_item.second);
				//TODO FIXME
				if(action->GetPrototype().GetIRSignature().GetName() == "instruction_predicate") continue;
				str << "inline " << action->GetPrototype().GetIRSignature().GetType().GetCType() << " " << action->GetPrototype().GetIRSignature().GetName() << "(";
				bool first = true;
				for(unsigned i = 0; i < action->ParamSymbols.size(); ++i) {
					if(!first) str << ", ";
					first = false;
					gensim::genc::ssa::SSASymbol *param_sym = action->ParamSymbols.at(i);
					str << param_sym->GetType().GetCType() << " " << param_sym->GetName();
				}
				str << ") {";
				GenerateExecuteBodyFor(str, *action);
				str << "}";
			}

			return true;
		}

		bool GenCInterpreterGenerator::GenerateExternHelperFns(util::cppformatstream &str) const
		{
			//TODO: actually emit non inline versions of functions
			return true;
		}

		bool GenCInterpreterGenerator::GenerateExtraProcessorSource(util::cppformatstream &str) const
		{
			// First, generate non-tracing
			const arch::ArchDescription::ISAListType isalist = Manager.GetArch().ISAs;

			str << "#include \"util/Vector.h\"\n";

			str << "#undef GENSIM_TRACE\n";
			str << "#include \"gensim/gensim_processor_api.h\"\n";
			for (auto isa_ci : isalist) {
				const isa::ISADescription &isa = *isa_ci;
				for (auto behaviour_i : isa.ExecuteActions) {
					std::string behaviour_name = behaviour_i.first;
					str << "void Processor::execute_" << isa.ISAName << "_" << behaviour_name << "(const Decode &inst) {";
					if (isa.GetSSAContext().HasAction(behaviour_name)) {
						GenerateExecuteBodyFor(str, *dynamic_cast<const genc::ssa::SSAFormAction *>(isa.GetSSAContext().GetAction(behaviour_name)));
					}
					str << "}";
				}
			}

			str << "#define GENSIM_TRACE\n";
			str << "#include \"gensim/gensim_processor_api.h\"\n";
			// now generate tracing
			for (auto isa_ci : isalist) {
				const isa::ISADescription &isa = *isa_ci;
				for (auto behaviour_i : isa.ExecuteActions) {
					std::string behaviour_name = behaviour_i.first;
					str << "void Processor::execute_" << isa.ISAName << "_" << behaviour_name << "_trace(const Decode &inst) {";
					if (isa.GetSSAContext().HasAction(behaviour_name)) {
						GenerateExecuteBodyFor(str, *dynamic_cast<const genc::ssa::SSAFormAction *>(isa.GetSSAContext().GetAction(behaviour_name)));
					}
					str << "}";
				}
			}

			return true;
		}
	}
}
