/*
 * NaiveJitGenerator.cpp
 *
 *  Created on: 24 Mar 2015
 *      Author: harry
 */

#include "generators/InterpretiveExecutionEngineGenerator.h"
#include "generators/TranslationGenerator.h"
#include "generators/MakefileGenerator.h"
#include "generators/NaiveJIT/NaiveJitWalker.h"

#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/statement/SSAStatement.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/Parser.h"

namespace gensim
{
	namespace generator
	{

		class NaiveJitGenerator : public TranslationGenerator
		{
		public:
			virtual ~NaiveJitGenerator();
			NaiveJitGenerator(GenerationManager &manager);

			bool Generate() const;

			void Setup(GenerationSetupManager &Setup);

		private:
			NaiveJitGenerator(const NaiveJitGenerator &orig) = delete;

			bool GenerateSource(std::ostringstream &str) const;

			bool GenerateEmitPredicate(std::ostringstream &str) const;
			bool GenerateTranslateInstruction(std::ostringstream &str) const;

			bool GeneratePrecomp() const;
		};


		DEFINE_COMPONENT(NaiveJitGenerator, translate_naive)
		COMPONENT_INHERITS(translate_naive, translate)
		COMPONENT_OPTION(translate_naive, Clang, "clang++", "compiler to use")
		COMPONENT_OPTION(translate_naive, Optimise, "0", "precompiled module optimisation level")

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
		static std::string GetLLVMTypeForLengthModule(uint32_t bits, const std::string module_name)
		{
			std::ostringstream str;

			//round bits up to nearest power of 2
			if(bits <= 8) bits = 8;
			else if(bits <= 16) bits = 16;
			else if(bits <= 32) bits = 32;
			else assert(false);

			str << "llvm::IntegerType::get(" << module_name << "->getContext(), " << bits << ")";
			return str.str();
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

		static std::string GenerateInstructionTypeFunction(const gensim::isa::ISADescription *isa)
		{
			std::ostringstream str;
			//output struct type is ISANAME_inst
			str << "llvm::StructType *GetStructType" << isa->ISAName << "(llvm::Module *mod) {"
			    "  llvm::StructType *ty = mod->getTypeByName(\"" << isa->ISAName << "_inst\");"
			    "  if(ty) return ty;"
			    "  ty = llvm::StructType::create(\"" << isa->ISAName << "_inst\", ";

			for(auto decode_field : isa->Get_Decode_Fields()) {
				str << GetLLVMTypeForLengthModule(decode_field.second,"mod") << ", ";
			}

			str <<
			    "NULL);"
			    "return ty;"
			    "}";
			return str.str();
		}

		static std::string GetOrInsertFunction(std::string fn_name, std::string rtype, const gensim::isa::ISADescription *isa)
		{
			std::ostringstream str;
			str << "insn.block.region.txln.llvm_module->getOrInsertFunction(\"" << fn_name << "\", insn.block.region.txln.types." << rtype;
			//functions take a cpu state pointer, a gensim_state pointer, a PC, and an instruction structure
			str << ",insn.block.region.txln.types.state_ptr";
			str << ",insn.block.region.txln.types.reg_state_ptr";
			str << ",insn.block.region.txln.types.i32";

			for(auto field : isa->Get_Decode_Fields()) {
				str << ", " << GetLLVMTypeForLengthModule(field.second, "insn.block.region.txln.llvm_module");
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

			str << "insn.block.region.builder.CreateCall(" << fn_name << ",";
			//functions take a cpu state pointer, a gensim_state pointer, and an instruction structure
			str << "std::vector<llvm::Value*>{insn.block.region.values.state_val";
			str << ",insn.block.region.values.reg_state_val";
			str << ",insn.block.region.CreateMaterialise(insn.tiu.GetOffset())";

			for(auto field : isa->Get_Decode_Fields()) {
				str << "," << GenerateLLVMConstant(field.second, "dec." +field.first);
			}
			str << "});";

			return str.str();
		}

		NaiveJitGenerator::~NaiveJitGenerator() {}

		NaiveJitGenerator::NaiveJitGenerator(GenerationManager &manager) : TranslationGenerator(manager, "translate_naive") {}

		bool NaiveJitGenerator::Generate() const
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

		void NaiveJitGenerator::Setup(GenerationSetupManager &Setup)
		{
			MakefileGenerator *makefile_gen = (MakefileGenerator *)Setup.GetComponent("make");
			if (makefile_gen != NULL) {
				makefile_gen->AddObjectFile("precompiled_insns.o");
				makefile_gen->AddPreBuildStep("objcopy --input binary --output elf64-x86-64 --binary-architecture i386 precompiled_insns.bc precompiled_insns.o");
			}
		}


		bool NaiveJitGenerator::GenerateSource(std::ostringstream &str) const
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

		bool NaiveJitGenerator::GenerateEmitPredicate(std::ostringstream &str) const
		{
			const arch::ArchDescription &arch = Manager.GetArch();

			for(auto isa : Manager.GetArch().ISAs) {
				str << GenerateInstructionTypeFunction(isa);
			}

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

		bool NaiveJitGenerator::GenerateTranslateInstruction(std::ostringstream &str) const
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

		static std::string GetFunctionSignature(const isa::ISADescription *isa, const std::string rtype, const std::string funname)
		{
			std::ostringstream str;
			str << rtype << " " << funname << "(struct cpuState* cpu, struct gensim_state *regs, uint32_t __pc";

			for(auto field : isa->Get_Decode_Fields()) {
				str << "," << GetCTypeForLength(field.second) << " __insn_" << field.first;
			}

			for(auto field : isa->UserFields) {
				str << "," << field.field_type << " __insn_" << field.field_name;
			}

			str << ")";
			return str.str();
		}

		bool NaiveJitGenerator::GeneratePrecomp() const
		{
			InterpretiveExecutionEngineGenerator *interp = (InterpretiveExecutionEngineGenerator *)Manager.GetComponentC(GenerationManager::FnInterpret);
			const auto &arch = Manager.GetArch();
			std::ofstream precomp(".precomp.c");

			util::cppformatstream str;

			str << "#include <cstdio>\n";
			str << "#include <cstdlib>\n";  // needed for abort()
			str << "#include <cstdint>\n";  // needed for abort()
			str << "#include \"" << GetProperty("archsim_path") << "/inc/gensim/gensim_processor_state.h\"\n";
			str << "#include \"" << GetProperty("archsim_path") << "/inc/translate/jit_funs.h\"\n";
			str << "#include \"" << GetProperty("archsim_path") << "/inc/util/Vector.h\"\n";
			//      stream << Manager.GetArch().GetIncludes();
			//
			//      stream << QUOTEME(TYPEDEF_TYPES); //todo: memory and pipeline modelling
			//      stream << QUOTEME(ENUM_MEMORY_TYPE_TAGS);

			str << "typedef uint32_t uint32; typedef uint16_t uint16; typedef uint8_t uint8; typedef int32_t sint32; typedef int16_t sint16; typedef int8_t sint8;";
			str << "typedef uint64_t uint64; typedef int64_t sint64;";

			str << interp->GetStateStruct();

			//generate code to read registers and reg banks
			for(const auto &reg : arch.GetRegFile().GetSlots()) {
				str << reg->GetIRType().GetCType() << " read_register_" << reg->GetID() << "(struct gensim_state *state) { return *(" << reg->GetIRType().GetCType() << "*)(((uint8_t*)state) + " << reg->GetRegFileOffset() << "); }";
				str << "void write_register_" << reg->GetID() << "(struct gensim_state *state, uint32_t value) { *(" << reg->GetIRType().GetCType() << "*)(((uint8_t*)state) + " << reg->GetRegFileOffset() << ") = value; }";
			}

			for(const auto &regbank : arch.GetRegFile().GetBanks()) {
				str << regbank->GetRegisterIRType().GetCType() << " read_register_bank_" << regbank->ID << "(struct gensim_state *state, uint32_t idx) { return *(" << regbank->GetRegisterIRType().GetCType() << "*)(((uint8_t*)state)+" << regbank->GetRegSpaceOffset() << " + " << regbank->GetRegisterStride() << " * idx); }";
				str << "void write_register_bank_" << regbank->ID << "(struct gensim_state *state, uint32_t idx, " << regbank->GetRegisterIRType().GetCType() << " value) { *(" << regbank->GetRegisterIRType().GetCType() << "*)(((uint8_t*)state)+" << regbank->GetRegSpaceOffset() << " + " << regbank->GetRegisterStride() << " * idx) = value; }";
			}

			str << "extern \"C\" uint32_t read_pc(const cpuState *cpu, const gensim_state *regs);";

			str << "extern \"C\" uint32_t mem_read_32(const cpuState *cpu, const uint32_t addr, uint32_t data);";
			str << "extern \"C\" uint32_t mem_read_16(const cpuState *cpu, const uint32_t addr, uint32_t data);";
			str << "extern \"C\" uint32_t mem_read_8(const cpuState *cpu, const uint32_t addr, uint32_t  data);";

			str << "extern \"C\" uint32_t mem_write_32(const cpuState *cpu, const uint32_t addr, uint32_t data);";
			str << "extern \"C\" uint32_t mem_write_16(const cpuState *cpu, const uint32_t addr, uint16_t data);";
			str << "extern \"C\" uint32_t mem_write_8(const cpuState *cpu, const uint32_t addr, uint8_t data);";

			str << "extern \"C\" uint32_t fn_mem_read_32(gensim::Processor *cpu, const uint32_t addr, uint32_t *data);";
			str << "extern \"C\" uint32_t fn_mem_read_16(gensim::Processor *cpu, const uint32_t addr, uint32_t *data);";
			str << "extern \"C\" uint32_t fn_mem_read_8(gensim::Processor *cpu, const uint32_t addr, uint32_t *data);";

			str << "extern \"C\" uint32_t fn_mem_write_32(gensim::Processor *cpu, const uint32_t addr, uint32_t data);";
			str << "extern \"C\" uint32_t fn_mem_write_16(gensim::Processor *cpu, const uint32_t addr, uint32_t data);";
			str << "extern \"C\" uint32_t fn_mem_write_8(gensim::Processor *cpu, const uint32_t addr, uint32_t data);";

			std::map<std::string, std::string> helper_macro_defs;

			for(auto isa : arch.ISAs) {
				//forward declare helpers

				for(auto behaviour : isa->HelperFns) {
					if(behaviour.name == "instruction_predicate") continue;

					//figure out macro def mapping
					std::stringstream macro;
					macro << behaviour.name << "(cpu, regs, __pc";
					for(int i = 3; i < behaviour.params.size(); ++i) macro << ", %" <<i-3 << " ";
					macro << ")";

					std::string macro_str = macro.str();
					helper_macro_defs[behaviour.name] = macro_str;

					str << behaviour.GetPrototype() << ";";
				}
			}

			for(auto isa : arch.ISAs) {
				const arch::RegSlotViewDescriptor *pc_slot = Manager.GetArch().GetRegFile().GetTaggedRegSlot("PC");
				const arch::RegSlotViewDescriptor *sp_slot = Manager.GetArch().GetRegFile().GetTaggedRegSlot("SP");

				// emit PC read and write functions
				str << "extern \"C\" uint32_t " << isa->ISAName << "_fast_read_pc(struct cpuState * const __state, struct gensim_state *const regs) {\n";
				str << "return *(" << pc_slot->GetIRType().GetCType() << "*)(((uint8_t*)regs) + " << pc_slot->GetRegFileOffset() << ");";
				str << "\n}\n";

				str << "extern \"C\" void " << isa->ISAName << "_fast_write_pc(struct cpuState *const __state, struct gensim_state *const regs, uint32_t val) {\n";
				str << "*(" << pc_slot->GetIRType().GetCType() << "*)(((uint8_t*)regs) + " << pc_slot->GetRegFileOffset() << ") = val;";
				str << "\n}\n";

				for(auto behaviour : isa->HelperFns) {
					if(behaviour.name == "instruction_predicate") continue;

					str << behaviour.GetPrototype();

					std::string behaviour_str = behaviour.body;

					for(const auto macro : helper_macro_defs) behaviour_str = inline_macro(behaviour_str, macro.first, macro.second, false);
					str << do_macros(behaviour_str, false);
				}

				//Emit instruction implementations
				for(auto insn : isa->Instructions) {
					using namespace genc::ssa;

					str << "extern \"C\" " << GetFunctionSignature(isa, "void", isa->ISAName + "_inst_" + insn.first) << "{";

					std::string behaviour = isa->GetExecuteAction(insn.second->BehaviourName);

					for(const auto macro : helper_macro_defs) behaviour = inline_macro(behaviour, macro.first, macro.second, false);
					str << do_macros(behaviour, false);

					str << "}";

				}

			}

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				if(!isa->HasBehaviourAction("instruction_predicate"))continue;

				str << "extern \"C\" " << GetFunctionSignature(isa, "char", isa->ISAName + "_predicate") << "{";

				str << do_macros(isa->GetBehaviourAction("instruction_predicate"), false);
				str << "}";


			}

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
			std::vector<std::string> include_dirs;// = Manager.GetArch().IncludeDirs;
			for (std::vector<std::string>::const_iterator idir = include_dirs.begin(); idir != include_dirs.end(); idir++) {
				cline_str << " -I output/" << *idir;
			}
			cline_str << " -std=c++11 -emit-llvm -I" << GetProperty("archsim_path") << "/inc -c "
			          << ".precomp.c"
			          << " -o " << Manager.GetTarget() << "/precompiled_insns.bc ";

			if (HasProperty("Optimise")) {
				if (GetProperty("Optimise") == "1")
					cline_str << " -O1";
				else if (GetProperty("Optimise") == "2")
					cline_str << " -O2";
				else if (GetProperty("Optimise") == "3")
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

	}
}

