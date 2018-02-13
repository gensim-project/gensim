/*
 * File:   ClangLLVMTranslationGenerator.cpp
 * Author: s0803652
 *
 * Created on 29 November 2011, 13:58
 */

#include <fstream>
#include <iomanip>
#include <vector>

#include "clean_defines.h"

#include "arch/ArchDescription.h"
#include "generators/ClangLLVMTranslationGenerator.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"
#include "generators/TranslationGenerator.h"
#include "isa/ISADescription.h"
#include "generators/MakefileGenerator.h"

#include "Util.h"
#define _QUOTEME(...) #__VA_ARGS__
#define QUOTEME(...) _QUOTEME(__VA_ARGS__)


DEFINE_COMPONENT(gensim::generator::ClangLLVMTranslationGenerator, translate_llvm)
COMPONENT_INHERITS(translate_llvm, translate)
COMPONENT_OPTION(translate_llvm, Optimize, "0", "Set the level of optimisation to use on the precompiled LLVM module")
COMPONENT_OPTION(translate_llvm, Debug, "0", "Enable/disable debug symbols in the precompiled LLVM module")
COMPONENT_OPTION(translate_llvm, InlineThreshold, "100", "Set the inlining threshold to use when optimizing the precompiled LLVM module")
COMPONENT_OPTION(translate_llvm, Clang, "clang++", "Path to the clang binary to use when compiling the precompiled LLVM module")

using namespace gensim::generator;

ClangLLVMTranslationGenerator::~ClangLLVMTranslationGenerator()
{

}

bool ClangLLVMTranslationGenerator::Generate() const
{

	std::ofstream hstr (Manager.GetTarget() + std::string("/translate.h"));
	util::cppformatstream hstream;
	GenerateHeader(hstream);
	hstr << hstream.str();

	std::ofstream sstr(Manager.GetTarget() + std::string("/translate.cpp"));
	util::cppformatstream sstream;
	GenerateSource(sstream);
	sstr << sstream.str();

	GeneratePrecomp();

	return true;
}

void ClangLLVMTranslationGenerator::Setup(GenerationSetupManager &Setup)
{
	MakefileGenerator *makefile_gen = (MakefileGenerator *)Setup.GetComponent("make");
	if (makefile_gen != NULL) {
		makefile_gen->AddObjectFile("precompiled_insns.o");
		makefile_gen->AddPreBuildStep("objcopy --input binary --output elf64-x86-64 --binary-architecture i386 precompiled_insns.bc precompiled_insns.o");
	}
}

bool ClangLLVMTranslationGenerator::GenerateSource(std::ostringstream &str) const
{
	str << "#include \"translate.h\"\n";
	str << "#include \"decode.h\"\n";

	str << "#include <translate/llvm/LLVMTranslationContext.h>\n";
	str << "#include <translate/TranslationWorkUnit.h>\n";

	str << "#include <llvm/IR/Module.h>\n";

	str <<
	    "extern \"C\""
	    "{"
	    "  extern uint8_t _binary_precompiled_insns_bc_start;"
	    "  extern uint8_t _binary_precompiled_insns_bc_end;"
	    "}";

	str << "using namespace gensim::" << Manager.GetArch().Name << ";";

	str << "Translate::Translate(const gensim::Processor &cpu, archsim::translate::llvm::LLVMTranslationContext &ctx) : BaseLLVMTranslate(cpu,ctx) {}";

	str <<
	    "uint8_t *Translate::GetPrecompBitcode()"
	    "{"
	    "  return &_binary_precompiled_insns_bc_start;"
	    "}"
	    "uint32_t Translate::GetPrecompSize()"
	    "{"
	    "  return &_binary_precompiled_insns_bc_end - &_binary_precompiled_insns_bc_start;"
	    "}";

	GenerateEmitPredicate(str);
	GenerateTranslateInstruction(str);

	GenerateJumpInfo(str);

	return true;
}

static std::string GetLLVMTypeForLength(uint8_t bits)
{
	if(bits <= 8) {
		return "insn.block.region.txln.types.i8";
	} else if(bits <= 16) {
		return "insn.block.region.txln.types.i16";
	} else if(bits <= 32) {
		return "insn.block.region.txln.types.i32";
	} else {
		assert(false);
		__builtin_unreachable();
	}
}

static std::string GetCTypeForLength(uint8_t bits)
{
	if(bits <= 8) {
		return "uint8_t";
	} else if(bits <= 16) {
		return "uint16_t";
	} else if(bits <= 32) {
		return "uint32_t";
	} else {
		assert(false);
		__builtin_unreachable();
	}
}

static std::string GetOrInsertFunction(std::string fn_name, std::string rtype, const gensim::isa::ISADescription *isa)
{
	std::ostringstream str;
	str << "insn.block.region.txln.llvm_module->getOrInsertFunction(\"" << fn_name << "\", insn.block.region.txln.types." << rtype;
	for(auto field : isa->Get_Decode_Fields()) {
		str << ", " << GetLLVMTypeForLength(field.second);
	}
	str << ", NULL);";
	return str.str();
}

static std::string GeneratePredFunPrototype(const gensim::isa::ISADescription *isa)
{
	return GetOrInsertFunction(isa->ISAName + "_predicate", "i8", isa);
}

static std::string GenerateLLVMConstant(std::string type, std::string value)
{
	return "llvm::ConstantInt::get(" + type + "," + value +", false)";
}

static std::string GenerateLLVMConstant(uint8_t bits, std::string value)
{
	return GenerateLLVMConstant(GetLLVMTypeForLength(bits), value);
}

static std::string GenerateFunCall(std::string fn_name, const gensim::isa::ISADescription *isa)
{
	std::ostringstream str;
	str << "insn.block.region.builder.CreateCall(" << fn_name << ", std::vector<llvm::Value*> ({";

	bool first = true;
	for(auto field : isa->Get_Decode_Fields()) {
		if(!first) str << ", ";
		first = false;

		str << GenerateLLVMConstant(field.second, "dec." +field.first);
	}
	str << "}))";
	return str.str();
}

bool ClangLLVMTranslationGenerator::GenerateEmitPredicate(std::ostringstream &str) const
{
	const arch::ArchDescription &arch = Manager.GetArch();

	str <<
	    "bool Translate::EmitPredicate(archsim::translate::llvm::LLVMInstructionTranslationContext& insn, ::llvm::Value*& __result, bool trace)"
	    "{ "
	    "  " << arch.Name << "::Decode &dec = (" << arch.Name << "::Decode &)insn.tiu.GetDecode();"
	    "  switch(dec.isa_mode) {";

	for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
		const isa::ISADescription *isa = *II;

		str <<
		    "case ISA_MODE_" << isa->ISAName << ": {"
		    "  llvm::Function *pred_fn = (llvm::Function*)" << GeneratePredFunPrototype(isa) <<
		    "  __result = " << GenerateFunCall("pred_fn", isa) << ";"
		    "  return true; "
		    "}";
	}

	str <<
	    "  }"
	    "  return false; "
	    "}";

	return true;
}

static void GenerateTranslateInstructionForIsa(std::ostringstream &str, const gensim::isa::ISADescription *isa)
{
	str << "switch(dec.Instr_Code) {";

	for(auto insn : isa->Instructions) {
		str <<
		    "case INST_" << isa->ISAName << "_" << insn.first << ":"
		    "{"
		    "	llvm::Function *fn = (llvm::Function*)" << GetOrInsertFunction(isa->ISAName + "_inst_" + insn.first, "vtype", isa) << ";"
		    "  " << GenerateFunCall("fn", isa) << ";"
		    "  return true;"
		    "}";
	}

	str <<
	    "	default: return false;"
	    "}";

}

bool ClangLLVMTranslationGenerator::GenerateTranslateInstruction(std::ostringstream &str) const
{
	const arch::ArchDescription &arch = Manager.GetArch();

	str << "bool Translate::TranslateInstruction(archsim::translate::llvm::LLVMInstructionTranslationContext& insn, llvm::Value*&, bool trace)"
	    "{"
	    "  " << arch.Name << "::Decode &dec = (" << arch.Name << "::Decode &)insn.tiu.GetDecode();"
	    "  switch(insn.tiu.GetDecode().isa_mode) {";
	for(auto isa : Manager.GetArch().ISAs) {
		str << "case " << isa->isa_mode_id << ":";
		GenerateTranslateInstructionForIsa(str, isa);
		str << "return true;";
	}

	str <<
	    "  }"
	    "  return false;"
	    "}";

	return true;
}

bool ClangLLVMTranslationGenerator::GeneratePrecomp() const
{
	InterpretiveExecutionEngineGenerator *interp = (InterpretiveExecutionEngineGenerator *)Manager.GetComponentC(GenerationManager::FnInterpret);
	const auto &arch = Manager.GetArch();
	std::ofstream precomp(".precomp.c");

	util::cppformatstream str;

	str << "#include <cstdio>\n";
	str << "#include <cstdlib>\n";  // needed for abort()
	str << "#include <cstdint>\n";  // needed for abort()
	str << "#include \"" << GetProperty("archsim_path") << "/inc/gensim/gensim_processor_state.h\"\n";
	//      stream << Manager.GetArch().GetIncludes();
	//
	//      stream << QUOTEME(TYPEDEF_TYPES); //todo: memory and pipeline modelling
	//      stream << QUOTEME(ENUM_MEMORY_TYPE_TAGS);

	str << "typedef uint32_t uint32; typedef uint16_t uint16; typedef uint8_t uint8; typedef int32_t sint32; typedef int16_t sint16; typedef int8_t sint8;";

	str << interp->GetStateStruct();

	str << "#define read_register_bank(bank,rnum) (regs->bank[rnum])\n";
	str << "#define read_register_bank_nt(bank,rnum) (regs->bank[rnum])\n";

	str << "#define write_register_bank(bank,rnum, val) (regs->bank[rnum] = (val))\n";
	str << "#define write_register_bank_nt(bank,rnum, val) (regs->bank[rnum] = (val))\n";

	str << "#define read_register(reg) (regs->reg)\n";
	str << "#define read_register_nt(reg) (regs->reg)\n";

	str << "#define write_register(reg, val) (regs->reg = (val))\n";
	str << "#define write_register_nt(reg, val) (regs->reg = (val))\n";


	for(auto isa : arch.ISAs) {

		//Emit instruction struct
		str << "struct " << isa->ISAName << "_inst {";

		for(auto field : isa->Get_Decode_Fields()) {
			str << "const " << GetCTypeForLength(field.second) << " " << field.first << ";";
		}

		for(auto field : isa->UserFields) {
			str << "const " << field.field_type << " " << field.field_name << ";";
		}

		str << "};";

		for(auto helper : isa->HelperFns) {
			str << helper.GetPrototype() << "{" << helper.body << "}";
		}

		//Emit instruction implementations
		for(auto insn : isa->Instructions) {

			str << "extern \"C\" void " << isa->ISAName << "_inst_" << insn.first << "(struct cpuState* cpu, struct gensim_state *regs, const struct " << isa->ISAName << "_inst inst){";

			str << isa->GetExecuteAction(insn.second->BehaviourName);

			str << "}";

		}

	}

//	for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
//		const isa::ISADescription *isa = *II;

//		if (isa->HasBehaviourAction("instruction_begin")) {
//			emit_behaviour_fn(stream, *isa, "pre", isa->GetBehaviourAction("instruction_begin"), false, "void");
//			emit_behaviour_fn(stream, *isa, "pre", isa->GetBehaviourAction("instruction_begin"), true, "void");
//		}
//
//		emit_behaviour_fn(stream, *isa, "instruction_predicate", isa->GetBehaviourAction("instruction_predicate"), false, "uint8");
//		emit_behaviour_fn(stream, *isa, "instruction_predicate", isa->GetBehaviourAction("instruction_predicate"), true, "uint8");
//
//		if (isa->HasBehaviourAction("instruction_end")) {
//			emit_behaviour_fn(stream, *isa, "post", isa->GetBehaviourAction("instruction_end"), false, "void");
//			emit_behaviour_fn(stream, *isa, "post", isa->GetBehaviourAction("instruction_end"), true, "void");
//		}
//
//		// emit PC read and write functions
//		stream << "inline uint32_t " << isa->ISAName << "_fast_read_pc(struct cpuState * const __state, struct gensim_state *const regs) {\n";
//		stream << do_macros(isa->GetBehaviourAction("read_pc"), false);
//		stream << "\n}\n";
//
//		stream << "inline void " << isa->ISAName << "_fast_write_pc(struct cpuState *const __state, struct gensim_state *const regs, uint32_t val) {\n";
//		stream << do_macros(isa->GetBehaviourAction("write_pc"), false);
//		stream << "\n}\n";
//	}

	// emit ISA mode read and write functions
	str << "inline uint8_t fast_get_isa_mode(struct cpuState * const __state) {\n";
	str << "\treturn __state->isa_mode;";
	str << "\n}\n";

	str << "inline void fast_set_isa_mode(struct cpuState *const __state, uint8_t val) {\n";
	str << "\t__state->isa_mode = val;";
	str << "\n}\n";

	precomp << str.str();
	precomp.close();


	std::stringstream cline_str;
	cline_str << GetProperty("Clang");
	std::vector<std::string> include_dirs = Manager.GetArch().IncludeDirs;
	for (std::vector<std::string>::const_iterator idir = include_dirs.begin(); idir != include_dirs.end(); idir++) {
		cline_str << " -I output/" << *idir;
	}
	cline_str << " -std=c++11 -emit-llvm -I" << GetProperty("archsim_path") << "/inc -c "
	          << ".precomp.c"
	          << " -o " << Manager.GetTarget() << "/precompiled_insns.bc ";

	if (HasProperty("Optimize")) {
		if (GetProperty("Optimize") == "1")
			cline_str << " -O1";
		else if (GetProperty("Optimize") == "2")
			cline_str << " -O2";
		else if (GetProperty("Optimize") == "3")
			cline_str << " -O3";
		else
			cline_str << " -O0";
	}

	if (HasProperty("InlineThreshold")) {
		cline_str << " -mllvm -inline-threshold=" << GetProperty("InlineThreshold") << " ";
		printf("[TRANSLATE] Using inline threshold %s\n", GetProperty("InlineThreshold").c_str());
	}

	if (HasProperty("InlineHint-Threshold")) {
		cline_str << " -mllvm -inlinehint-threshold=" << GetProperty("InlineHint-Threshold") << " ";
		printf("[TRANSLATE] Using inline hint threshold %s\n", GetProperty("InlineHint-Threshold").c_str());
	}

	fprintf(stderr, "[TRANSLATE] Invoking clang with %s...", cline_str.str().c_str());

	if (system(cline_str.str().c_str())) {
		fprintf(stderr, "LLVM Compilation failed\n");
		return false;
	}
	fprintf(stderr, "(done)\n");

	return true;
}
