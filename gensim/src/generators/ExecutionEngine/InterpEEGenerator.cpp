/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/ExecutionEngine/InterpEEGenerator.h"
#include "generators/GenCInterpreter/GenCInterpreterGenerator.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::generator;

bool InterpEEGenerator::GenerateHeader(util::cppformatstream &str) const {
	str << 
		"#include \"decode.h\"\n"
		"#include <gensim/ExecutionEngine.h>\n"
		"#include <cstdint>\n"
		
		"class EE : public archsim::BasicExecutionEngine {"
		"public:"
		
		"	using decode_t = gensim::" << Manager.GetArch().Name << "::Decode;"
		
		"	archsim::ExecutionResult StepThreadSingle(archsim::ThreadInstance *thread);"
		"	archsim::ExecutionResult StepThreadBlock(archsim::ThreadInstance *thread);"
		
		"private:"
		"  uint32_t DecodeInstruction(archsim::ThreadInstance *thread, decode_t &inst);"
		"  archsim::ExecutionResult StepInstruction(archsim::ThreadInstance *thread, decode_t &inst);"
		"};"
		;
	return true;
}

bool InterpEEGenerator::GenerateSource(util::cppformatstream &str) const {
	str <<
		"#include \"ee_interpreter.h\"\n"
		"#include \"arch.h\"\n"
		"#include \"decode.h\"\n"
		"#include <module/Module.h>\n"
		"#include <util/Vector.h>\n"
		"#include <translate/jit_funs.h>\n"
		;
	
	GenerateDecodeInstruction(str);
	
	str <<
		"archsim::ExecutionResult EE::StepThreadSingle(archsim::ThreadInstance *thread) { return archsim::ExecutionResult::Abort; }"
		"archsim::ExecutionResult EE::StepThreadBlock(archsim::ThreadInstance *thread) { ";
	
	GenerateBlockExecutor(str);
	
	str <<
		"}";

	GenerateStepInstruction(str);
	
	str <<
		
		"ARCHSIM_MODULE() {"
		"  auto module = new archsim::module::ModuleInfo(\"" << Manager.GetArch().Name << "\", \"CPU Module\");"
		"  auto ee_entry = new archsim::module::ModuleExecutionEngineEntry(\"EE\", ARCHSIM_EEFACTORY(EE));"
		
		"  auto arch_entry = new archsim::module::ModuleArchDescriptorEntry(\"ArchDescriptor\", ARCHSIM_ARCHDESCRIPTORFACTORY(gensim::" << Manager.GetArch().Name << "::ArchDescriptor));"
		
		"	module->AddEntry(ee_entry);"
		"	module->AddEntry(arch_entry);"
		"  return module;"
		"}"
		"ARCHSIM_MODULE_END() {}"
		;
		
	return true;
}

bool InterpEEGenerator::GenerateDecodeInstruction(util::cppformatstream& str) const
{
	str << "uint32_t EE::DecodeInstruction(archsim::ThreadInstance *thread, EE::decode_t &inst) {";
	
	str << "  return inst.DecodeInstr(thread->GetPC(), thread->GetModeID(), thread->GetFetchMI());";
	
	str << "}";
	
	return true;
}


bool InterpEEGenerator::GenerateBlockExecutor(util::cppformatstream& str) const
{
	str << 
		"while(true) {"
	
		"  decode_t inst;"
		"  uint32_t dcode_exception = DecodeInstruction(thread, inst);"
		"  auto result = StepInstruction(thread, inst);"
		"  if(result != archsim::ExecutionResult::Continue) { break; }"
		"  if(inst.GetEndOfBlock()) { break; }"
		"  thread->SetPC(thread->GetPC() + inst.Instr_Length);"
		
		"}";
	
	return true;
}

bool InterpEEGenerator::GenerateStepInstruction(util::cppformatstream& str) const
{
	for(auto i : Manager.GetArch().ISAs) {
		GenerateStepInstructionISA(str, *i);
	}
	
	str << 
		"archsim::ExecutionResult EE::StepInstruction(archsim::ThreadInstance *thread, EE::decode_t &inst) {";
	
	str << "switch(thread->GetModeID()) {";
	
	for(auto i : Manager.GetArch().ISAs) {
		str << "case " << i->isa_mode_id << ": if(archsim::options::Trace) { return StepInstruction_" << i->ISAName << "<true>(thread, inst); } else { return StepInstruction_" << i->ISAName << "<false>(thread, inst); }";
	}
		
	str << 
		"  default: "
		"    LC_ERROR(LogExecutionEngine) << \"Unknown mode\"; break;"
		"  }";
	
	str << 
		"  return archsim::ExecutionResult::Abort; "
		"}";
	
	return true;
}

bool InterpEEGenerator::GenerateStepInstructionInsn(util::cppformatstream& str, isa::InstructionDescription& insn) const
{
	str << "template<bool trace=false> archsim::ExecutionResult StepInstruction_" << insn.ISA.ISAName << "_" << insn.Name << "(archsim::ThreadInstance *thread, EE::decode_t &inst) {";
	str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	
	str << "if(trace) { if(thread->GetTraceSource() == nullptr) { throw std::logic_error(\"\"); } thread->GetTraceSource()->Trace_Insn(thread->GetPC().Get(), inst.ir, false, thread->GetModeID(), thread->GetExecutionRing(), 1); }";
	
	gensim::generator::GenCInterpreterGenerator gci (Manager);
	gci.GenerateExecuteBodyFor(str, *static_cast<const gensim::genc::ssa::SSAFormAction*>(insn.ISA.GetSSAContext().GetAction(insn.BehaviourName)));
	
	str << "if(trace) { thread->GetTraceSource()->Trace_End_Insn(); }";
	str << "return archsim::ExecutionResult::Continue;";
	str << "}";
	
	return true;
}

bool InterpEEGenerator::GenerateStepInstructionISA(util::cppformatstream& str, isa::ISADescription& isa) const
{
	for(auto i : isa.Instructions) {
		GenerateStepInstructionInsn(str, *i.second);
	}
	
	str << "template<bool trace=false> archsim::ExecutionResult StepInstruction_" << isa.ISAName << "(archsim::ThreadInstance *thread, EE::decode_t &decode) {";
	
	str << " switch(decode.Instr_Code) {";
	str << " using namespace gensim::" << Manager.GetArch().Name << ";";
	
	for(auto i : isa.Instructions) {
		str << "case INST_" << isa.ISAName << "_" << i.second->Name << ": return StepInstruction_" << isa.ISAName << "_" << i.first << "<trace>(thread, decode);";
	}
	
	str << " default: LC_ERROR(LogExecutionEngine) << \"Unknown instruction\"; return archsim::ExecutionResult::Abort;";
	
	str << "}";
	str << "}";
	
	return true;
}


InterpEEGenerator::~InterpEEGenerator()
{

}


DEFINE_COMPONENT(InterpEEGenerator, ee_interp)