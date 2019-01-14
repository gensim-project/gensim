/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/GenerationManager.h"
#include "Util.h"

#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSASymbol.h"
#include "generators/GenCJIT/DynamicTranslationGenerator.h"


namespace gensim
{
	namespace generator
	{
		class LLVMTranslatorGenerator : public GenerationComponent
		{
		public:
			LLVMTranslatorGenerator(GenerationManager &man) : GenerationComponent(man, "llvm_translator")
			{
				GenerateModuleEntry();
			}

			virtual ~LLVMTranslatorGenerator()
			{

			}

			bool GenerateHeader() const
			{
				util::cppformatstream str;
				str << "#pragma once\n";
				str << "#include <gensim/gensim_translate.h>\n";
				str << "#include <translate/llvm/LLVMTranslationContext.h>\n";

				str << "#include <llvm/IR/IRBuilder.h>\n";
				str << "#include <llvm/IR/BasicBlock.h>\n";
				str << "#include <llvm/IR/Function.h>\n";

				str << "class LLVMTranslate" << Manager.GetArch().Name << " : public gensim::BaseLLVMTranslate {";
				str << "public:";
				str << "virtual ~LLVMTranslate" << Manager.GetArch().Name << "();";
				str << "virtual bool TranslateInstruction(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext &ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn) override;";
				str << "virtual llvm::Value *EmitPredicateCheck(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn) override;";

				str << "private:";
				for(auto i : Manager.GetArch().ISAs) {
					str << "bool Translate_" << i->ISAName << "(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext &ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn);";

					for(auto insn : i->Instructions) {
						str << "bool Translate_" << i->ISAName << "_" << insn.second->Name << "(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext &ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn);";
					}
				}

				str << "};";

				WriteOutputFile("llvm_translate.h", str);

				return true;
			}

			bool GenerateDispatch() const
			{
				std::string prototype = "bool LLVMTranslate" + Manager.GetArch().Name + "::TranslateInstruction(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext &ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn)";

				util::cppformatstream str;
				str << prototype;
				str << "{";

				str << "switch(decode->isa_mode) {";

				for(auto isa : Manager.GetArch().ISAs) {
					str << "case " << isa->isa_mode_id << ": return Translate_" << isa->ISAName << "(builder, ctx, thread, decode, phys_pc, fn);";
				}

				str << "default: throw std::logic_error(\"Unexpected isa\");";

				str << "}";

				str << "}";
				std::vector<std::string> headers = {"llvm_translate.h"};

				Manager.AddFunctionEntry(FunctionEntry(prototype, str.str(), headers));

				return true;
			}

			bool GeneratePredicateChecker(gensim::isa::ISADescription &isa) const
			{
				std::string prototype = "llvm::Value *LLVMTranslate" + Manager.GetArch().Name + "::EmitPredicateCheck(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext& ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn)";
				util::cppformatstream str, header;
				str << prototype << " { ";

				str << "::llvm::Module *module = fn->getParent();";
				str << "::llvm::LLVMContext &llvm_ctx = module->getContext();";
				str << "::llvm::IRBuilder<> &__irBuilder = builder;";
				str << "::llvm::Value *__result = nullptr;";
				str << "bool __trace = false;";

				DynamicTranslationGenerator dtg (Manager);
				auto ssa_action = (gensim::genc::ssa::SSAFormAction*)isa.GetSSAContext().GetAction("instruction_predicate");

				str << "auto &" << ssa_action->ParamSymbols.at(0)->GetName() << " = *(const gensim::" << Manager.GetArch().Name << "::Decode *)decode;";
				dtg.EmitDynamicEmitter(str, header, *(gensim::genc::ssa::SSAFormAction*)ssa_action, "lol");

				str << "return __result;";
				str << " }";

				Manager.AddFunctionEntry(FunctionEntry(prototype, str.str(), {"llvm_translate.h"}));

				return true;
			}

			bool GenerateTranslatorFor(gensim::isa::InstructionDescription &insn) const
			{
				std::string prototype = "bool LLVMTranslate" + Manager.GetArch().Name + "::Translate_" + insn.ISA.ISAName + "_" + insn.Name + "(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext &ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn)";
				util::cppformatstream str, header;
				str << prototype << "{";

				str << "::llvm::Module *module = fn->getParent();";
				str << "::llvm::LLVMContext &llvm_ctx = module->getContext();";
				str << "::llvm::IRBuilder<> &__irBuilder = builder;";
				str << "::llvm::Value *__result = nullptr;";
				str << "bool __trace = false;";

				auto ssa_action = (gensim::genc::ssa::SSAFormAction*)insn.ISA.GetSSAContext().GetAction(insn.BehaviourName);
				str << "auto &" << ssa_action->ParamSymbols.at(0)->GetName() << " = *(const gensim::" << Manager.GetArch().Name << "::Decode *)decode;";

				DynamicTranslationGenerator dtg (Manager);
				dtg.EmitDynamicEmitter(str, header, *(gensim::genc::ssa::SSAFormAction*)ssa_action, "lol");

				str << " return true;";
				str << 	"}";

				std::vector<std::string> headers = {"llvm_translate.h"};

				Manager.AddFunctionEntry(FunctionEntry(prototype, str.str(), headers));

				return true;
			}

			bool GenerateTranslatorFor(gensim::isa::ISADescription &isa) const
			{
				std::string prototype = "bool LLVMTranslate" + Manager.GetArch().Name + "::Translate_" + isa.ISAName + "(Builder &builder, archsim::translate::translate_llvm::LLVMTranslationContext &ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn)";

				util::cppformatstream str;
				str << prototype;
				str << "{";

				str << "switch(decode->Instr_Code) {";
				str << "using namespace gensim::" << Manager.GetArch().Name << ";";

				for(auto insn_ : isa.Instructions) {
					gensim::isa::InstructionDescription *insn = insn_.second;
					str << "case INST_" << isa.ISAName << "_" << insn->Name << ": return Translate_" << isa.ISAName << "_" << insn->Name << "(builder, ctx, thread, decode, phys_pc, fn);";
				}

				str << "default: throw std::logic_error(\"Unexpected instruction code\");";

				str << "}";
				str << "}";

				Manager.AddFunctionEntry(FunctionEntry(prototype, str.str(), {"llvm_translate.h"}));

				for(auto insn : isa.Instructions) {
					GenerateTranslatorFor(*insn.second);
				}

				return true;
			}

			bool GenerateTranslators() const
			{
				// generate isa dispatch functions
				for(auto isa : Manager.GetArch().ISAs) {
					GenerateTranslatorFor(*isa);
					GeneratePredicateChecker(*isa);
				}

				return true;
			}

			bool GenerateModuleEntry() const
			{
				Manager.AddModuleEntry(ModuleEntry("LLVMTranslator", "LLVMTranslate" + Manager.GetArch().Name, "llvm_translate.h", ModuleEntryType::LLVMTranslator));

				return true;
			}

			bool Generate() const override
			{
				std::string prototype = "LLVMTranslate"+Manager.GetArch().Name+"::~LLVMTranslate"+Manager.GetArch().Name + "()";
				Manager.AddFunctionEntry(FunctionEntry(prototype,prototype + "{}", {"llvm_translate.h"}));

				GenerateHeader();

				GenerateDispatch();
				GenerateTranslators();

				return true;
			}
			std::string GetFunction() const override
			{
				return "";
			}
			const std::vector<std::string> GetSources() const override
			{
				return {};
			}

		};
	}
}

DEFINE_COMPONENT(gensim::generator::LLVMTranslatorGenerator, llvm_translator)
