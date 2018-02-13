/*
 * File:   InterpretiveExecutionEngineGenerator.cpp
 * Author: s0803652
 *
 * Created on 14 October 2011, 14:34
 */

#include <sstream>
#include <fstream>

#include "define.h"
#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/InterpretiveExecutionEngineGenerator.h"
#include "generators/ClangLLVMTranslationGenerator.h"
#include "generators/DecodeGenerator.h"
#include "DecodeTree.h"

DEFINE_COMPONENT(gensim::generator::InterpretiveExecutionEngineGenerator, interpret)
COMPONENT_INHERITS(interpret, ee_gen)

COMPONENT_OPTION(interpret, class, "Processor", "Class name for interpreter component")
COMPONENT_OPTION(interpret, ErrorContinue, "0", "Specifies whether to continue generation if certain kinds of error are encountered and instead fail at runtime")
COMPONENT_OPTION(interpret, StaticPredication, "0", "Set to en1 if an instruction can be determined to be predicated at decode time (e.g. standard ARM predication) rather than at runtime (e.g. ARMv7-M IT blocks)")
COMPONENT_OPTION(interpret, DecodeCacheSize, "8192", "The number of entries in the decode cache. Each entry has 2 ways.")
COMPONENT_OPTION(interpret, PCWriteCheck, "0", "Insert checks to make sure that branch instructions are correctly detected")
COMPONENT_OPTION(interpret, PCReadCheck, "0", "Insert checks to make sure that reads from the PC are correctly detected")
COMPONENT_OPTION(interpret, PCReadCheckReg, "0", "The register to check for PC reads")
COMPONENT_OPTION(interpret, CheckEndOfBlock, "0", "Check for set_end_of_block violations")
COMPONENT_OPTION(interpret, Instrument, "0", "Instrument block and instruction execution")
COMPONENT_OPTION(interpret, ParentClass, "gensim::Processor", "")

namespace gensim
{
	namespace generator
	{

		InterpretiveExecutionEngineGenerator::~InterpretiveExecutionEngineGenerator() {}

		bool InterpretiveExecutionEngineGenerator::Generate() const
		{
			bool success = true;
			util::cppformatstream header_str;
			util::cppformatstream source_str;

			// generate the source streams
			success &= GenerateHeader(header_str);
			success &= GenerateSource(source_str);

			if (success) {
				WriteOutputFile("processor.h", header_str);
				WriteOutputFile("processor.cpp", source_str);
			}
			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateInlineHelperFns(util::cppformatstream &stream) const
		{
			stream << "#define PASS_REGS REGS_T &reg_state\n";
			stream << "#define PASS_STATE cpuState * const s\n";

			// TODO: Review this multi-isa stuff - because it's semi wrong.
			isa::ISADescription *isa = Manager.GetArch().ISAs.front();

			for (std::vector<isa::HelperFnDescription>::const_iterator i = isa->HelperFns.begin(); i != isa->HelperFns.end(); ++i) {
				if(i->name == "instruction_predicate") {
					continue;
				}
				if (i->should_inline) stream << "/*inline*/  inline " << i->GetPrototype() << "\n" << i->body << "\n\n";
			}

			stream << "#undef PASS_STATE\n";
			stream << "#undef PASS_REGS\n";
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExternHelperFns(util::cppformatstream &source_stream) const
		{
			source_stream << "#define PASS_REGS REGS_T &reg_state\n";
			source_stream << "#define PASS_STATE cpuState * const s\n";

			// TODO: Review this multi-isa stuff - because it's semi wrong.
			isa::ISADescription *isa = Manager.GetArch().ISAs.front();

			for (std::vector<isa::HelperFnDescription>::const_iterator i = isa->HelperFns.begin(); i != isa->HelperFns.end(); ++i) {
				if(i->name == "instruction_predicate") {
					continue;
				}
				if (!i->should_inline) source_stream << "/*noninline*/ inline " << i->GetPrototype() << "\n" << i->body << "\n\n";
			}
			source_stream << "#undef PASS_STATE\n";
			source_stream << "#undef PASS_REGS\n";
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateHeader(util::cppformatstream &stream) const
		{
			bool success = true;
			const arch::ArchDescription &arch = Manager.GetArch();
			const DecodeGenerator *decode = static_cast<const DecodeGenerator *>(Manager.GetComponentC(GenerationManager::FnDecode));
			const GenerationComponent *disasm = Manager.GetComponentC(GenerationManager::FnDisasm);
			const GenerationComponent *txlt = Manager.GetComponentC(GenerationManager::FnTranslate);

			stream << ""
			       "/* Auto generated header file for Interpretive Execution Engine for " << arch.Name << " */\n\n"
			       "#ifndef __" << arch.Name << "_" << GetProperty("class") << "\n"
			       "#define __" << arch.Name << "_" << GetProperty("class") << "\n"
			       "#include <iomanip>\n"
			       "#include <unistd.h>\n"
			       "#include <stdlib.h>\n"
			       "#include <setjmp.h>\n"
			       "#include <string.h>\n" << arch.GetIncludes() << "#define INTERP\n"
			       "#include <gensim/gensim_processor_api.h>\n"
			       "#undef INTERP\n"
			       "#include <gensim/gensim_processor.h>\n"
			       "#include <tracing/TraceManager.h>\n"
			       "#include <util/Cache.h>\n"
			       "#include \"decode.h\"\n"
			       "#include \"disasm.h\"\n"
			       "#include \"jumpinfo.h\"\n"
			       "#include <arch/arm/ARMDecodeContext.h>\n"
			       "#include <util/LogContext.h>\n";

			if(txlt) stream << "#include \"translate.h\"\n";

			GenerateExtraProcessorIncludes(stream);

			stream << "UseLogContext(LogCPU);\n";

			stream << "extern char **environ;\n" << GetStateStruct() << ""
			       "namespace gensim {\n"
			       "  class BaseDisasm;\n"
			       "  namespace " << arch.Name << "{\n"
			       "    class Processor : public " << GetProperty("ParentClass") << "\n{\n"
			       "public:\n";

			stream << "struct gensim_state reg_state;\n";
			stream << "dynamic_pred_queue_t dynamic_predication;\n";
			GenerateExtraProcessorClassMembers(stream);

			stream << "// Inline helpers\n";
			GenerateInlineHelperFns(stream);
			stream << "// Non-inline helpers\n";
			GenerateExternHelperFns(stream);

			// TODO: Review this multi-isa stuff - because it's semi wrong.
			isa::ISADescription *static_isa = Manager.GetArch().ISAs.front();

			stream << ""
			       "Processor(const std::string &, int _core_id, archsim::util::PubSubContext* ctx);\n"
			       "~Processor();"
			       "inline void halt_cpu() { Halt(); }\n"
			       "bool step_single();\n"
			       "bool step_single_fast();\n"
			       "bool step_instruction(bool trace);\n"
			       "bool step_block_fast();\n"
			       "bool step_block_trace();\n"
			       "bool step_block_fast_thread();\n";

			const arch::RegSlotViewDescriptor *pc_slot = Manager.GetArch().GetRegFile().GetTaggedRegSlot("PC");
			const arch::RegSlotViewDescriptor *sp_slot = Manager.GetArch().GetRegFile().GetTaggedRegSlot("SP");

			stream <<
			       "inline uint32_t read_pc() { return *(" << pc_slot->GetIRType().GetCType() << "*)(((uint8_t*)state.gensim_state) + " << pc_slot->GetRegFileOffset() << "); }\n"
			       "inline void write_pc(uint32_t val) { *(" << pc_slot->GetIRType().GetCType() << "*)(((uint8_t*)state.gensim_state) + " << pc_slot->GetRegFileOffset() << ") = val; }\n"
			       "inline uint32_t read_sp() { return *(" << sp_slot->GetIRType().GetCType() << "*)(((uint8_t*)state.gensim_state) + " << sp_slot->GetRegFileOffset() << "); }\n"
			       "inline void write_sp(uint32_t val) { *(" << sp_slot->GetIRType().GetCType() << "*)(((uint8_t*)state.gensim_state) + " << sp_slot->GetRegFileOffset() << ") = val; }\n";

			stream <<
			       "void software_interrupt(uint32_t category, uint32_t nr);"
			       "void trace_status(std::stringstream &trace);";

			stream << "uint32_t GetRegisterFileSize() override { return " << arch.GetRegFile().GetSize() << ";}";

			if(static_isa->HasBehaviourAction("fetch_exception")) {
				stream << "void handle_fetch_fault(uint32_t fault) override {" << static_isa->GetBehaviourAction("fetch_exception") << "}\n";
			}

			if (static_isa->HasBehaviourAction("reset")) {
				stream << "void reset_exception();\n";
			}

			if(static_isa->HasBehaviourAction("irq")) {
				stream << "virtual void handle_irq(uint32_t line);";
			}

			if(static_isa->HasBehaviourAction("handle_exception")) {
				stream << "void handle_exception(uint32_t category, uint32_t data);";
			}

			// todo: place a reference to the current instruction in this function
			// TODO: Review static predication stuff for multi-isa
			if (static_isa->HasBehaviourAction("is_predicated")) {
				stream << "bool curr_insn_is_predicated() {";
				if (GetProperty("StaticPredication") == "1") {
					stream << "return curr_interp_insn->GetIsPredicated();";
				} else {
					stream << static_isa->GetBehaviourAction("is_predicated");
				}
				stream << "}\n";
			} else {
				stream << "bool curr_insn_is_predicated() { return true; }\n";
			}

			stream << ""
					"gensim::DecodeContext *GetDecodeContext() { if(decode_context_ == nullptr) decode_context_ = GetEmulationModel().GetNewDecodeContext(*this); return decode_context_; }"
			       "gensim::BaseDecode *curr_interpreted_insn() { return curr_interp_insn; }\n"
			       "void restart_from_halted() { halted = false; }\n"
			       "void reset() { " << GetProperty("ParentClass") << "::reset(); }\n"
			       "bool reset_to_initial_state(bool purge) { " << GetProperty("ParentClass") << "::reset_to_initial_state(purge);return true; }\n"
			       "void purge_dcode_cache();\n"
			       "gensim::BaseDisasm *GetDisasm();\n"

			       "gensim::BaseLLVMTranslate *CreateLLVMTranslate(archsim::translate::llvm::LLVMTranslationContext& ctx) const;\n"
			       "gensim::BaseIJTranslate *CreateIJTranslate(archsim::ij::IJTranslationContext& ctx) const;\n"

			       "void InitialiseFeatures() override;\n"

			       "gensim::BaseJumpInfo *GetJumpInfo() const;\n"
			       "gensim::BaseDecode *GetNewDecode();\n"
			       "Decode insn_decode;\n"
			       "uint32_t DecodeInstr(uint32_t pc, uint8_t isa_mode, BaseDecode &dec);"
			       "void DecodeInstrIr(uint32_t ir, uint8_t isa_mode, BaseDecode &dec);"
			       "private:\n" << disasm->GetProperty("class") << " disasm;\n"
			       "Decode *curr_interp_insn;\n"
					"DecodeContext *decode_context_;"
			       "archsim::util::Cache<" << decode->GetProperty("class") << ", " << GetProperty("DecodeCacheSize") << "> decode_cache;\n"
			       "uint32_t decode_instruction_using_cache(uint32_t pc, uint8_t isa_mode, " << decode->GetProperty("class") << " *&dcode, uint32_t &efa);\n";

			// XXX ARM HAX
			stream << "archsim::arch::arm::ARMDecodeContext *_arm_decode_ctx;";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				if (isa->HasBehaviourAction("instruction_predicate")) stream << "inline bool " << isa->ISAName << "_instruction_predicate(" << decode->GetProperty("class") << "& inst)\n{\n" << isa->GetBehaviourAction("instruction_predicate") << "\n}\n";
			}

			GenerateRegisterAccessFunctions(stream);


			stream << "// hack to allow reading memory from helper functions\n"
			       "inline cpuState *GetState() { return &state; }\n"
			       "};\n"
			       "} }\n"
			       "#endif\n";

			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateRegisterAccessFunctions(util::cppformatstream &str) const
		{
			const gensim::arch::RegisterFile &regfile = Manager.GetArch().GetRegFile();

			//emit register bank access functions
			for(const auto &bank : regfile.GetBanks()) {
				auto register_type = bank->GetRegisterIRType();
				
				// Read register function

				str << register_type.GetCType() << " read_register_bank_" << bank->ID << "(uint32_t reg_idx) {";

				str << "assert(reg_idx < " << bank->GetRegisterCount() << ");";
				str << "assert(sizeof(" << register_type.GetCType() << ") == " << bank->GetRegisterWidth() << ");"; //todo: fix this so that width can be less than size of type

				str << "uint8_t *bank_ptr = (uint8_t*)state.gensim_state + " << bank->GetRegFileOffset() << ";";
				str << "uint8_t *stride_ptr = bank_ptr + (reg_idx * " << bank->GetRegisterStride() << ");";
				str << bank->GetRegisterIRType().GetCType() << "* entry_ptr = (" << register_type.GetCType() << "*)stride_ptr;";

				if(bank->GetElementCount() == 1) {
					if(register_type.IsFloating()) {
						str << "float f_val = *entry_ptr;";
						str << "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_Bank_Reg_Read(true, TRACE_ID_" << bank->ID << ", reg_idx, *(uint32_t*)(&f_val));";
					} else {
						str << "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_Bank_Reg_Read(true, TRACE_ID_" << bank->ID << ", reg_idx, *entry_ptr);";
					}
				}

				str << "return *entry_ptr;";
				str << "}";

				// Write register function

				str << "void write_register_bank_" << bank->ID << "(uint32_t reg_idx, " << register_type.GetCType() << " value) {";

				str << "assert(reg_idx < " << bank->GetRegisterCount() << ");";
				str << "assert(sizeof(" << register_type.GetCType() << ") == " << bank->GetRegisterWidth() << ");"; //todo: fix this so that width can be less than size of type

				str << "uint8_t *bank_ptr = (uint8_t*)state.gensim_state + " << bank->GetRegFileOffset() << ";";
				str << "uint8_t *stride_ptr = bank_ptr + (reg_idx * " << bank->GetRegisterStride() << ");";

				str << register_type.GetCType() << "* entry_ptr = (" << register_type.GetCType() << "*)stride_ptr;";
				str << "*entry_ptr = value;";

				// TODO: Proper support for tracing floats and vectors
				if(bank->GetElementCount() == 1) {
					if(register_type.IsFloating()) {
						str << "float f_val = value;";
						str << "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_Bank_Reg_Write(true, TRACE_ID_" << bank->ID << ", reg_idx, *(uint32_t*)(&f_val));";
					} else {
						str << "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_Bank_Reg_Write(true, TRACE_ID_" << bank->ID << ", reg_idx, value);";
					}
				}

				str << "}";
			}

			//
			for(const auto &slot : regfile.GetSlots()) {
				//Read function
				str << slot->GetIRType().GetCType() << " read_register_" << slot->GetID() << "() {";
				str << "assert(sizeof(" << slot->GetIRType().GetCType() << ") == " << slot->GetWidth() << ");"; //todo: fix this so that width can be less than size of type

				str << "uint8_t *reg_ptr = (uint8_t*)state.gensim_state + " << slot->GetRegFileOffset() << ";";
				str << slot->GetIRType().GetCType() << "* entry_ptr = (" << slot->GetIRType().GetCType() << "*)reg_ptr;";
				str << "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_Reg_Read(true, TRACE_ID_" << slot->GetID() << ", *entry_ptr);";
				str << "return *entry_ptr;";
				str << "}";

				str << "void write_register_" << slot->GetID() << "(" << slot->GetIRType().GetCType() << " value) {";
				str << "assert(sizeof(" << slot->GetIRType().GetCType() << ") == " << slot->GetWidth() << ");"; //todo: fix this so that width can be less than size of type

				str << "uint8_t *reg_ptr = (uint8_t*)state.gensim_state + " << slot->GetRegFileOffset() << ";";
				str << slot->GetIRType().GetCType() << "* entry_ptr = (" << slot->GetIRType().GetCType() << "*)reg_ptr;";
				str << "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_Reg_Write(true, TRACE_ID_" << slot->GetID() << ", value);";
				str << "*entry_ptr = value;";
				str << "}";

			}

			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExecutionForBehaviour(util::cppformatstream &stream, bool trace_enabled, std::string behaviour_name, const isa::ISADescription &isa) const
		{
			// Check arch for vector registers: this interpreter doesn't support them
			auto &regfile = Manager.GetArch().GetRegFile();
			for(auto &bank : regfile.GetBanks()) {
				if(bank->GetElementCount() != 1) {
					fprintf(stderr, "Error! Standard interpreter does not support vector register banks. Please use genc_interpret instead.\n");
					return false;
				}
			}

			if(GetProperty("CheckEndOfBlock") == "1") stream << "{uint32_t _pre_pc = read_pc();";
			stream << "{" << *(isa.ExecuteActions.at(behaviour_name)) << "}\n";
			if(GetProperty("CheckEndOfBlock") == "1")stream << "if(!inst.GetEndOfBlock() && read_pc() != _pre_pc) fprintf(stderr, \"END OF BLOCK VIOLATION %x: %s\\n\", pre_pc, GetDisasm()->DisasmInstr(inst, _pre_pc).c_str()); }";
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExecution(util::cppformatstream &stream, bool trace_enabled, std::string out_field) const
		{
			bool success = true;

			const arch::ArchDescription &arch = Manager.GetArch();
			const GenerationComponent *decode = Manager.GetComponentC(GenerationManager::FnDecode);
			const GenerationComponent *cycle = Manager.GetComponentC(GenerationManager::FnPipeline);

			if (GetProperty("PCReadCheck") == "1") {
				stream << "#define GENSIM_PC_CHECK\n";
				stream << "#define GENSIM_PC_REG " << GetProperty("PCReadCheckReg") << "\n";
			}
			if (trace_enabled) stream << "#define GENSIM_TRACE\n";
			stream << "#include <gensim/gensim_processor_api.h>\n";
			if (trace_enabled) stream << "#undef GENSIM_TRACE\n";

			//Run a verify check before we execute each instruction
			stream << "if(archsim::options::Verify && !_skip_verify) GetEmulationModel().GetSystem().CheckVerify();\n";
			stream << "_skip_verify = false;";

			stream << "uint32_t pre_pc = read_pc();\n";
			stream << "uint8_t isa_mode = get_cpu_mode();\n";
			stream << "uint32_t efa = 0;\n";

			stream << "uint32_t ecause;";
			stream << "curr_interp_insn = &insn_decode;";
			stream << "Decode &inst = *((Decode*)curr_interp_insn);\n";
			stream << "if(ecause = GetDecodeContext()->DecodeSync(archsim::translate::profile::Address(pre_pc), get_cpu_mode(), *curr_interp_insn)) {";
			stream << "	handle_fetch_fault(ecause);";
			stream << "} else {";

			stream << "end_of_block = inst.GetEndOfBlock();\n";

			if(GetProperty("Instrument") == "1") {
				stream << "struct { uint32_t pc, count; } block_ctx; block_ctx.pc = pre_pc; block_ctx.count = 1;";
				stream << "GetSubscriber()->Publish(PubSubType::BlockExecute, (void*)&block_ctx);";
			}

			if (trace_enabled) {
				stream << "if (HasTraceManager())\n";
				stream << "GetTraceManager()->Trace_Insn(pre_pc, inst.GetIR(), 0, isa_mode, interrupt_mode, 1);";
			}

			stream << "last_exception_action = archsim::abi::None;\n";
			stream << "bool execute_insn = true;\n";

			stream << "switch (isa_mode) {\n";
			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				stream << "case ISA_MODE_" << isa->ISAName << ": {\n";

				if (isa->HasBehaviourAction("instruction_begin")) {
					stream << isa->GetBehaviourAction("instruction_begin");
				}

				if (isa->HasBehaviourAction("instruction_predicate")) {
					if (isa->HasBehaviourAction("is_predicated")) {
						stream << "if(curr_insn_is_predicated()) "
						       "{ ";

//						if (GetProperty("StaticPredication") == "0") stream << "   phys_profile_.get_page_profile(pre_pc, zone_)->predicated_insns.insert(pre_pc); ";

						stream << "   inst.SetIsPredicated(); "
						       "   execute_insn = " << isa->ISAName << "_instruction_predicate(inst);"
						       "} "
						       "else "
						       "{"
						       "   inst.ClearIsPredicated();"
						       "   execute_insn = true;"
						       "}\n";
					} else {
						stream << "execute_insn = instruction_predicate(inst);\n";
					}
				}

				stream << "} break;\n";
			}

			stream << "}\n";

			stream << "if(execute_insn)"
			       "{"
			       "  switch(inst.Instr_Code){";

			// first, group instructions by their behaviours
			std::map<std::string, std::list<const isa::InstructionDescription *> > instrs;

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;
				for (std::map<std::string, std::string *>::const_iterator i = isa->ExecuteActions.begin(); i != isa->ExecuteActions.end(); ++i) {
					instrs[i->first] = std::list<const isa::InstructionDescription *>();
				}

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					if (instrs.find(i->second->BehaviourName) == instrs.end()) {
						fprintf(stderr, "Late-caught error: Unknown behaviour %s required by instruction %s\n", i->second->BehaviourName.c_str(), i->first.c_str());
						if (GetProperty("ErrorContinue") == "1") {
							continue;
						} else {
							success = false;
							continue;
						}
					}
					instrs[i->second->BehaviourName].push_back(i->second);
				}
			}

			if (!success) return success;

			for (std::map<std::string, std::list<const isa::InstructionDescription *> >::iterator i = instrs.begin(); i != instrs.end(); ++i) {
				if (i->second.size() == 0) continue;  // if there are no instructions using this behaviour, don't output it

				// Theoretically, instructions can only be grouped by behaviour when they're part
				// of the same ISA.  So, getting the first ISA from the grouped list is fine here.
				const isa::ISADescription &isa = i->second.front()->ISA;

				for (std::list<const isa::InstructionDescription *>::iterator inst = i->second.begin(); inst != i->second.end(); ++inst) {
					stream << "case INST_" << isa.ISAName << "_" << (*inst)->Name << ":\n";
				}
				if (isa.ExecuteActions.at(i->first) == NULL) {
					if (GetProperty("ErrorContinue") == "1") {
						stream << "{ "
						       "  LC_ERROR(LogCPU) << \"Attempted to execute invalid instruction (code \" "
						       "    << inst.Instr_Code "
						       "    << \", \" "
						       "    << GetDisasm()->GetInstrName(inst.Instr_Code) "
						       "    << \") At PC 0x\" "
						       "    << std::hex << std::setw(8) << std::setfill('0') << read_pc() "
						       "    << \": \" "
						       "    << std::hex << std::setw(8) << std::setfill('0') << inst.GetIR(); "
						       "  Halt(); "
						       "}\n"
						       "break;\n";
						continue;
					} else {
						fprintf(stderr, "Late-caught error: Could not find definition for behaviour %s. Specify option ErrorContinue in order to continue generation.\n", i->first.c_str());
						success = false;
						continue;
					}
				}
				stream << "{";
				success &= GenerateExecutionForBehaviour(stream, trace_enabled, i->first, isa);
				stream << "} break;";
			}

			if(arch.ISAs.front()->HasBehaviourAction("undef")) {
				stream << "default : { end_of_block = true;"
				       "LC_DEBUG1(LogCPU) << \"Simulated core took undefined instruction exception mode=\" << (uint32_t)state.isa_mode << \", pc=\" << std::hex << read_pc() << \", code=\" << std::dec << (uint32_t)inst.Instr_Code << \", ir=\" << std::hex << inst.ir;"
				       << arch.ISAs.front()->GetBehaviourAction("undef") << "} break;";
			} else {
				stream << "default: { "
				       "  LC_ERROR(LogCPU) << \"Attempted to execute invalid instruction (code \" "
				       "    << inst.Instr_Code "
				       "    << \", \" "
				       "    << GetDisasm()->GetInstrName(inst.Instr_Code) "
				       "    << \") At PC 0x\" "
				       "    << std::hex << std::setw(8) << std::setfill('0') << read_pc() "
				       "    << \": \" "
				       "    << std::hex << std::setw(8) << std::setfill('0') << inst.GetIR(); "
				       "   Halt(); "
				       "}\n"
				       "break;\n";

			}
			stream << "}\n";

			stream << "switch (isa_mode) {\n";
			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				stream << "case ISA_MODE_" << isa->ISAName << ": {\n";

				if (isa->HasBehaviourAction("instruction_end")) stream << isa->GetBehaviourAction("instruction_end");

				stream << "} break;\n";
			}
			stream << "}\n";

			// ---------------------------------------------------------------------------
			// Profiling counters
			//
			stream << "  // Record PC frequencies\n"
			       "  //\n"
			       "  if(__builtin_expect(archsim::options::Profile, false)) {"
			       "    if (archsim::options::ProfilePcFreq) "
			       "    {"
			       "      metrics.pc_freq_hist.inc(pre_pc);\n"
			       "    }"
			       "    if(archsim::options::ProfileIrFreq) "
			       "    { "
			       "      metrics.inst_ir_freq_hist.inc(inst.GetIR());"
			       "    }"
			       "    // Increment instruction frequency histogram\n"
			       "    //\n"
			       "    metrics.opcode_freq_hist.inc(inst.Instr_Code);\n"
			       "  }"
			       "  // Increment interpreted instructions\n"
			       "  //\n"
			       "  metrics.interp_inst_count.inc();\n";


			if (GetProperty("PCWriteCheck") == "1") {
				stream << "if(pre_pc != read_pc() && (read_pc() != pre_pc + inst.Instr_Length) && !end_of_block)"
				       "{"
				       "    LC_ERROR(LogCPU) << \"Instruction at \" << std::hex << pre_pc << \" modified the PC but is not marked as PC-changing\";"
				       "    Halt();"
				       "}";
			}

			//Only increment the PC if:
			//1. The instruction is not an end-of-block
			//2. If the last_exception_action is None or ResumeNext
			stream << "  if(!end_of_block && ((last_exception_action == archsim::abi::ResumeNext) || (last_exception_action == archsim::abi::None))) write_pc(pre_pc + inst.Instr_Length);"
			       "} else {"  // end of execute_insn if statement: this block is executed if the instruction is predicated and does not execute
			       "  metrics.interp_inst_count.inc();"
			       "  write_pc(pre_pc + inst.Instr_Length);"
			       "}"
			       ;
			if (trace_enabled) stream << "\nif (HasTraceManager()) GetTraceManager()->Trace_End_Insn();\n";

			stream << "}";
			stream << out_field << " !halted && (last_exception_action != archsim::abi::AbortSimulation);\n";

			return success;
		}
		bool InterpretiveExecutionEngineGenerator::GenerateModuleDescriptor(util::cppformatstream& stream) const
		{
			stream << "ARCHSIM_MODULE() {";
			
			stream << "auto module = new archsim::module::ModuleInfo(\"" << Manager.GetArch().Name << "\", \"CPU Implementation Module\");";
			stream << "auto device_entry = new archsim::module::ModuleProcessorEntry(\"CPU\", ARCHSIM_PROCESSORFACTORY(gensim::" << Manager.GetArch().Name << "::Processor));";
			stream << "module->AddEntry(device_entry);";
			stream << "return module;";
			
			stream << "}";
			stream << "ARCHSIM_MODULE_END() { }";
			
			//stream << "RegisterComponent(gensim::Processor, Processor, \"" << Manager.GetArch().Name << "\", \"CPU Implementation\",  const std::string&, int, archsim::util::PubSubContext*);\n";
			return true;
		}

		
		bool InterpretiveExecutionEngineGenerator::GenerateSource(util::cppformatstream &stream) const
		{
			bool success = true;
			const arch::ArchDescription &arch = Manager.GetArch();
			const GenerationComponent *decode = Manager.GetComponentC(GenerationManager::FnDecode);
			const GenerationComponent *cycle = Manager.GetComponentC(GenerationManager::FnPipeline);

			stream << "/* Auto generated source file for Interpretive Execution Engine for " << arch.Name << " */\n";

			stream << "#include <stdio.h>\n";

			stream << "#include \"decode.h\"\n";
			stream << "#include \"disasm.h\"\n";
			stream << "#include \"processor.h\"\n";

			stream << "#include <util/Counter.h>\n";
			stream << "#include <util/Cache.h>\n";
			stream << "#include <util/Histogram.h>\n";
			stream << "#include <module/Module.h>\n";

			stream << "#include <util/ComponentManager.h>\n";
			stream << "#include <arch/arm/ARMDecodeContext.h>\n";

			GenerateModuleDescriptor(stream);
			
			stream << "namespace gensim {\n";
			stream << "namespace " << arch.Name << " {\n";

			// TODO: Review this multi-isa stuff - because it's semi wrong.
			isa::ISADescription *static_isa = Manager.GetArch().ISAs.front();

			stream << "Processor::Processor(const std::string &arch_name, int _core_id, archsim::util::PubSubContext* ctx)"
			       " : " << GetProperty("ParentClass") << "(\"" << arch.Name << "\", _core_id, ctx)"
			       "{"
			       "decode_cache.construct();\n"
			       "state.gensim_state = (GENSIM_STATE_PTR)&reg_state;\n"
			       "curr_interp_insn = NULL;"
			       "static_insn_predication = " << GetProperty("StaticPredication") << ";"
			       "memset(&reg_state, 0, sizeof(reg_state));\n";

			if (arch.big_endian)
				stream << "SetEndianness(archsim::abi::memory::BigEndian);\n";
			else
				stream << "SetEndianness(archsim::abi::memory::LittleEndian);\n";

			for (const auto &bank : arch.GetRegFile().GetBanks()) {
				stream << "register_banks[\"" << bank->ID << "\"] = RegisterBankDescriptor(\"" << bank->ID << "\", " << bank->GetRegFileOffset() << ", " << bank->GetRegisterCount() << ", " << bank->GetRegisterStride() << ", " << bank->GetElementCount() << ", " << bank->GetElementSize() << ", " << bank->GetElementStride() << ", " << "(void *)(((uint8_t*)state.gensim_state) + " << bank->GetRegFileOffset() << "));";
				stream << "register_banks_by_id[TRACE_ID_" << bank->ID << "] = register_banks[\"" << bank->ID << "\"];";
			}

			for (const auto &reg : arch.GetRegFile().GetSlots()) {
				stream << "registers[\"" << reg->GetID() << "\"].Name = \"" << reg->GetID() << "\";";
				stream << "registers[\"" << reg->GetID() << "\"].RegisterWidth = " << reg->GetWidth() << ";";
				stream << "registers[\"" << reg->GetID() << "\"].Offset = " << reg->GetRegFileOffset() << ";";

				if(reg->HasTag())
					stream << "registers[\"" << reg->GetID() << "\"].Tag = \"" << reg->GetTag() << "\";";

				stream << "registers[\"" << reg->GetID() << "\"].DataStart = (void *)(((uint8_t*)state.gensim_state) + " << reg->GetRegFileOffset() << ");";
				stream << "registers_by_id[TRACE_ID_" << reg->GetID() << "] = registers[\"" << reg->GetID() << "\"];";
			}

			GenerateExtraProcessorInitSource(stream);

			stream << "}\n";

			stream << "void Processor::InitialiseFeatures() {";
			for(const auto &feature : arch.GetFeatures()) {
				stream << "GetFeatures().AddNamedFeature(\"" << feature.GetName() << "\", " << feature.GetId() << ");";
				stream << "GetFeatures().SetFeatureLevel(\"" << feature.GetName() << "\", " << feature.GetDefaultLevel() << ");";
			}
			stream << "}";

			if (static_isa->HasBehaviourAction("reset")) {
				stream << "void Processor::reset_exception() { " << static_isa->GetBehaviourAction("reset") << "}";
			}

			if(static_isa->HasBehaviourAction("irq")) {
				stream << "void Processor::handle_irq(uint32_t irq_line) { " << static_isa->GetBehaviourAction("irq") << "}";
			}

			if(static_isa->HasBehaviourAction("handle_exception")) {
				stream << "void Processor::handle_exception(uint32_t category, uint32_t data) { " << static_isa->GetBehaviourAction("handle_exception") << "}";
			}

			stream << "Processor::~Processor() {";
			GenerateExtraProcessorDestructorSource(stream);
			stream << "}";

			stream << "void Processor::trace_status(std::stringstream &trace) {" << static_isa->GetBehaviourAction("get_status_string") << "}\n";

			stream << "gensim::BaseDisasm *Processor::GetDisasm() { return &disasm; }\n";
			stream << "gensim::BaseDecode *Processor::GetNewDecode() { return new Decode();}\n";

			stream << "inline uint32_t Processor::DecodeInstr(uint32_t pc, uint8_t isa_mode, BaseDecode &dec) "
			       "{"
			       "	Decode &d = static_cast<Decode&>(dec);"
			       "	uint32 result = d.DecodeInstr(pc, isa_mode, *this, this->dynamic_predication);"
			       "	if(!result) {"
			       "		"
			       "	}"
			       "   return 0;"
			       "}";

			stream << "void Processor::DecodeInstrIr(uint32_t ir, uint8_t isa_mode, BaseDecode &dec) { Decode &d = static_cast<Decode&>(dec); d.DecodeInstr(ir, isa_mode, this->dynamic_predication, false); } ";

			stream << "gensim::BaseLLVMTranslate *Processor::CreateLLVMTranslate(archsim::translate::llvm::LLVMTranslationContext& ctx) const {";
			if(Manager.GetComponentC(GenerationManager::FnTranslate))
				stream << "return new Translate(*this, ctx); }\n";
			else
				stream << "assert(false); return nullptr;";
			stream << "}";

			stream << "gensim::BaseIJTranslate *Processor::CreateIJTranslate(archsim::ij::IJTranslationContext& ctx) const { assert(false); }\n";
			stream << "gensim::BaseJumpInfo *Processor::GetJumpInfo() const { return new JumpInfo(); }";

			stream << "void  Processor::purge_dcode_cache() { decode_cache.purge(); }\n";

			stream << "Decode insn_decode;";

			stream << "inline uint32_t Processor::decode_instruction_using_cache(uint32_t pc, uint8_t isa_mode, " << decode->GetProperty("class") << " *&dcode, uint32_t &efa)"
			       "{\n"
			       "  uint32_t ecause = 0;\n";

			if (GetProperty("DecodeCacheSize") != "0") {
				stream <<
				       "  uint32_t ir;"
				       "  ecause = GetMemoryModel().Fetch32(pc, ir);"
				       "  if(ecause) return ecause;"

				       "  archsim::util::CacheHitType hit_type = decode_cache.try_cache_fetch(ir, dcode);\n"
				       "  if (archsim::util::CACHE_MISS == hit_type) {\n"
				       "    DecodeInstrIr(ir, isa_mode, *dcode);"
				       "  }\n";
			} else {
				stream << "dcode = &insn_decode; ecause = DecodeInstr(pc, isa_mode, *dcode);";
			}
			stream << "  return ecause;\n"
			       "}\n\n";

			success &= GenerateStepBlockTrace(stream);
			success &= GenerateStepBlockFast(stream);
			success &= GenerateStepBlockFastThread(stream);

			success &= GenerateStepSingle(stream);
			success &= GenerateStepSingleFast(stream);

			success &= GenerateStepInstruction(stream);

			success &= GenerateExtraProcessorSource(stream);

			stream << "} }\n";
			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateStepBlockTrace(util::cppformatstream &stream) const
		{
			bool success = true;
			stream << "bool Processor::step_block_trace() "
			       "{"
			       "     bool stepOK = true;"
			       " uint32_t _check_page_pc = archsim::translate::profile::RegionArch::PageIndexOf(read_pc());"
			       "metrics.interrupt_checks.inc();"
			       "handle_pending_action();"
			       "if(GetTraceManager() && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_End_Insn();"
			       "     do "
			       "     {"
			       " uint32_t _current_check_page_pc = archsim::translate::profile::RegionArch::PageIndexOf(read_pc());"
			       " if(_check_page_pc != _current_check_page_pc) break;";
			success &= GenerateExecution(stream, true, "stepOK = ");
			stream << "     }"
			       "     while (!end_of_block && stepOK && last_exception_action == archsim::abi::None && !halted); "

			       "return stepOK;}";

			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateStepBlockFast(util::cppformatstream &stream) const
		{
			bool success = true;
			stream << "bool Processor::step_block_fast() "
			       "{ "
			       "     bool stepOK = true;"
			       " uint32_t _check_page_pc = archsim::translate::profile::RegionArch::PageIndexOf(read_pc());"
			       "metrics.interrupt_checks.inc();"
			       "handle_pending_action();"
			       "     do "
			       "     {"
			       " uint32_t _current_check_page_pc = archsim::translate::profile::RegionArch::PageIndexOf(read_pc());"
			       " if(_check_page_pc != _current_check_page_pc) break;";
			success &= GenerateExecution(stream, false, "stepOK = ");

			stream << "     }"
			       "     while (!end_of_block && stepOK && last_exception_action == archsim::abi::None && !halted); "

			       "return stepOK;"
			       "}";

			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateThreadedDispatchCode(util::cppformatstream &stream) const
		{
			stream << "// Dispatch\n{";
			stream << "uint32_t efa = 0;";
			stream << "decode_instruction_using_cache(read_pc(), get_cpu_mode(), curr_interp_insn, efa);";
			stream << "inst = *((Decode*)curr_interp_insn);";
			stream << "end_of_block = inst.GetEndOfBlock();";
			stream << "goto *jump_table[inst.Instr_Code];";
			stream << "}";

			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateStepBlockFastThread(util::cppformatstream &stream) const
		{
			const arch::ArchDescription &arch = Manager.GetArch();

			stream << "bool Processor::step_block_fast_thread() {";
			stream << "return false;";
			stream << "}";

			return true;

#if 0
			stream << "void *jump_table[] = {";

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					stream << "&&L_INST_" << isa->ISAName << "_" << i->second->Name << ", ";
				}
			}

			stream << "};";

			stream << "#include <gensim/gensim_processor_api.h>\n";
			stream << "Decode& inst = *((Decode*)curr_interp_insn);";

			GenerateThreadedDispatchCode(stream);

			std::map<std::string, std::list<const isa::InstructionDescription *> > instrs;

			for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
				const isa::ISADescription *isa = *II;
				for (std::map<std::string, std::string*>::const_iterator i = isa->ExecuteActions.begin(); i != isa->ExecuteActions.end(); ++i) {
					instrs[i->first] = std::list<const isa::InstructionDescription*>();
				}

				for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
					if (instrs.find(i->second->BehaviourName) == instrs.end()) {
						fprintf(stderr, "Late-caught error: Unknown behaviour %s required by instruction %s\n", i->second->BehaviourName.c_str(), i->first.c_str());
						if (GetProperty("ErrorContinue") == "1") {
							continue;
						} else {
							return false;
						}
					}
					instrs[i->second->BehaviourName].push_back(i->second);
				}
			}

			for (std::map<std::string, std::list<const isa::InstructionDescription*> >::iterator GI = instrs.begin(), GE = instrs.end(); GI != GE; ++GI) {
				if (GI->second.size() == 0)
					continue; //if there are no instructions using this behaviour, don't output it

				for (std::list<const isa::InstructionDescription*>::iterator II = GI->second.begin(), IE = GI->second.end(); II != IE; ++II) {
					stream << "L_INST_" << (*II)->ISA.ISAName << "_" << (*II)->Name << ":\n";
				}

				const isa::ISADescription& isa = GI->second.front()->ISA;

				stream << "{ bool execute_insn = false;";
				stream << "// Predication\n{";
				stream << "if (curr_insn_is_predicated()) {";
				stream << "  inst.SetIsPredicated();";
				stream << "  execute_insn = " << isa.ISAName << "_instruction_predicate(*((Decode*)curr_interp_insn));";
				stream << "} else {";
				stream << "  inst.ClearIsPredicated();";
				stream << "  execute_insn = true;";
				stream << "}";
				stream << "}";

				stream << "// Behaviour\nif (execute_insn) {" << *(isa.ExecuteActions.at(GI->first)) << "}\n";
				stream << "}";

				stream << "if (end_of_block) return true;";

				GenerateThreadedDispatchCode(stream);
			}

			stream << "}";

			return true;
#endif
		}

		bool InterpretiveExecutionEngineGenerator::GenerateStepSingle(util::cppformatstream &stream) const
		{
			bool success = true;
			// emit step_single
			stream << "bool " << GetProperty("class") << "::step_single()\n{\n";
			stream << "uint32_t execution_result;";

			if(GetProperty("Instrument") == "1") {
				stream << "struct { uint32_t pc, code; } insn_ctx; ";//insn_ctx.pc = pre_pc;";// insn_ctx.code = inst.Instr_Code;";
				stream << "GetSubscriber()->Publish(PubSubType::InstructionExecute, (void*)&insn_ctx);";
			}

			success &= GenerateExecution(stream, true, "execution_result = ");
			stream << "metrics.interrupt_checks.inc();";
			stream << "handle_pending_action();";
			stream << "return execution_result;";
			stream << "}\n";
			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateStepSingleFast(util::cppformatstream &stream) const
		{
			bool success = true;
			// emit step_single_fast
			stream << "bool " << GetProperty("class") << "::step_single_fast()\n{\n";
			stream << "uint32_t execution_result;";

			if(GetProperty("Instrument") == "1") {
				stream << "struct { uint32_t pc, code; } insn_ctx; ";//insn_ctx.pc = pre_pc;";// insn_ctx.code = inst.Instr_Code;";
				stream << "GetSubscriber()->Publish(PubSubType::InstructionExecute, (void*)&insn_ctx);";
			}

			success &= GenerateExecution(stream, false, "execution_result = ");
			stream << "metrics.interrupt_checks.inc();";
			stream << "handle_pending_action();";
			stream << "return execution_result;";
			stream << "}\n";

			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateStepInstruction(util::cppformatstream &stream) const
		{
			bool success = true;
			// emit step_single
			stream << "bool " << GetProperty("class") << "::step_instruction(bool trace)\n{\n";
			stream << "if (trace) return step_single(); else return step_single_fast();";
			stream << "}\n";
			return success;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExtraProcessorClassMembers(util::cppformatstream &) const
		{
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExtraProcessorSource(util::cppformatstream &) const
		{
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExtraProcessorInitSource(util::cppformatstream &stream) const
		{
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExtraProcessorDestructorSource(util::cppformatstream &) const
		{
			return true;
		}

		bool InterpretiveExecutionEngineGenerator::GenerateExtraProcessorIncludes(util::cppformatstream &) const
		{
			return true;
		}

		const std::vector<std::string> InterpretiveExecutionEngineGenerator::GetSources() const
		{
			std::vector<std::string> sources;
			sources.push_back("processor.cpp");
			return sources;
		}

	}  // namespace generator
}  // namespace gensim
