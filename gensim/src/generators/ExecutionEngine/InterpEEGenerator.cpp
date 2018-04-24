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
#include "genC/ssa/SSASymbol.h"

using namespace gensim::generator;

bool InterpEEGenerator::GenerateHeader(util::cppformatstream &str) const {
	str << 
		"#include \"decode.h\"\n"
		"#include <gensim/ExecutionEngine.h>\n"
		"#include <cstdint>\n"
		
		"namespace " << Manager.GetArch().Name << "{"
		
		"class EE : public archsim::BasicExecutionEngine {"
		"public:"
		
		"	using decode_t = gensim::" << Manager.GetArch().Name << "::Decode;"
		
		"	archsim::ExecutionResult ArchStepSingle(archsim::ThreadInstance *thread);"
		"	archsim::ExecutionResult ArchStepBlock(archsim::ThreadInstance *thread);"
		
		"private:"
		"  uint32_t DecodeInstruction(archsim::ThreadInstance *thread, decode_t &inst);"
		"  archsim::ExecutionResult StepInstruction(archsim::ThreadInstance *thread, decode_t &inst);"
		"};"
		""
		;

		
	str << "}";
		
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
		"#include <gensim/gensim_processor_api.h>\n"
		"#include <abi/devices/Device.h>\n"
		"#include <cmath>\n"
		;
	
	str << "using namespace " << Manager.GetArch().Name << ";";
	
	GenerateDecodeInstruction(str);
	
	str <<
		"archsim::ExecutionResult EE::ArchStepSingle(archsim::ThreadInstance *thread) { return archsim::ExecutionResult::Abort; }"
		"archsim::ExecutionResult EE::ArchStepBlock(archsim::ThreadInstance *thread) { ";
	
	GenerateBlockExecutor(str);
	
	str <<
		"}";

	GenerateHelperFunctions(str);
	GenerateStepInstruction(str);
	
	GenerateBehavioursDescriptors(str);
	
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

void GenerateHelperFunctionPrototype(gensim::util::cppformatstream &str, const gensim::isa::ISADescription &isa, const gensim::genc::ssa::SSAFormAction *action) 
{
	str << "template<bool trace=false> " << action->GetPrototype().ReturnType().GetCType() << " helper_" << isa.ISAName << "_" << action->GetPrototype().GetIRSignature().GetName() << "(EE *ee, archsim::ThreadInstance *thread";

	for(auto i : action->ParamSymbols) {
		// if we're accessing a struct, assume that it's an instruction
		if(i->GetType().IsStruct()) {
			str << ", EE::decode_t &inst";
		} else {
			auto type_string = i->GetType().GetCType();
			str << ", " << type_string << " " << i->GetName();
		}
	}
	str << ")";
}

bool InterpEEGenerator::GenerateHelperFunctions(util::cppformatstream& str) const
{
	for(auto isa : Manager.GetArch().ISAs) {
		for(auto action : isa->GetSSAContext().Actions()) {
			if(action.second->GetPrototype().HasAttribute(gensim::genc::ActionAttribute::Helper)) {
				GenerateHelperFunctionPrototype(str, *isa, static_cast<gensim::genc::ssa::SSAFormAction*>(action.second));
				str << ";";
			}
		}
	}
	
	for(auto isa : Manager.GetArch().ISAs) {
		for(auto action : isa->GetSSAContext().Actions()) {
			if(action.second->GetPrototype().HasAttribute(gensim::genc::ActionAttribute::Helper)) {
				GenerateHelperFunction(str, *isa, static_cast<gensim::genc::ssa::SSAFormAction*>(action.second));
			}
		}
	}
	
	return true;
}

bool InterpEEGenerator::GenerateHelperFunction(util::cppformatstream& str, const gensim::isa::ISADescription &isa, const gensim::genc::ssa::SSAFormAction* action) const
{
	assert(action != nullptr);
	GenerateHelperFunctionPrototype(str, isa, action);
	str << " { ";

	str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	
	gensim::generator::GenCInterpreterGenerator gci (Manager);
	gci.GenerateExecuteBodyFor(str, *action);	
	
	str << " }";
	
	return true;
}

bool InterpEEGenerator::GenerateDecodeInstruction(util::cppformatstream& str) const
{
	str << "uint32_t EE::DecodeInstruction(archsim::ThreadInstance *thread, EE::decode_t &inst) {";
	str << "  gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	str << "  return inst.DecodeInstr(interface.read_pc(), thread->GetModeID(), thread->GetFetchMI());";
	
	str << "}";
	
	return true;
}


bool InterpEEGenerator::GenerateBlockExecutor(util::cppformatstream& str) const
{
	str << 
		"while(true) {"
		
		"  decode_t inst;"
		"  uint32_t dcode_exception = DecodeInstruction(thread, inst);"
		"  if(thread->HasMessage()) { return thread->HandleMessage(); }"
		"  if(dcode_exception) { thread->TakeMemoryException(thread->GetFetchMI(), thread->GetPC()); return archsim::ExecutionResult::Exception; }"
		"  auto result = StepInstruction(thread, inst);"
		"  if(inst.GetEndOfBlock()) { if(archsim::options::InstructionTick) { thread->GetPubsub().Publish(PubSubType::InstructionExecute, nullptr); } return archsim::ExecutionResult::Continue; }"
		"  if(result != archsim::ExecutionResult::Continue) { return result; }"
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
		str << "case " << i->isa_mode_id << ": if(archsim::options::Trace) { return StepInstruction_" << i->ISAName << "<true>(this, thread, inst); } else { return StepInstruction_" << i->ISAName << "<false>(this, thread, inst); }";
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
	str << "template<bool trace=false> archsim::ExecutionResult StepInstruction_" << insn.ISA.ISAName << "_" << insn.Name << "(EE *ee, archsim::ThreadInstance *thread, EE::decode_t &inst) {";
	str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	
	gensim::generator::GenCInterpreterGenerator gci (Manager);
	gci.GenerateExecuteBodyFor(str, *static_cast<const gensim::genc::ssa::SSAFormAction*>(insn.ISA.GetSSAContext().GetAction(insn.BehaviourName)));
	
	str << "return archsim::ExecutionResult::Continue;";
	str << "}";
	
	return true;
}

bool InterpEEGenerator::GenerateStepInstructionISA(util::cppformatstream& str, isa::ISADescription& isa) const
{
	for(auto i : isa.Instructions) {
		GenerateStepInstructionInsn(str, *i.second);
	}
	
	// predicate stuff
	bool has_is_predicated = isa.GetSSAContext().HasAction("instruction_is_predicated");
	bool has_instruction_predicate = isa.GetSSAContext().HasAction("instruction_predicate");
	if(has_is_predicated != has_instruction_predicate) {
		// bad times
	}
	if(has_is_predicated) {
		str << "bool " << isa.ISAName << "_is_predicated(EE *ee, archsim::ThreadInstance *thread, EE::decode_t &insn) { return helper_" << isa.ISAName << "_instruction_is_predicated(ee, thread, insn); }";
	}
	if(has_instruction_predicate) {
		str << "bool " << isa.ISAName << "_check_predicate(EE *ee, archsim::ThreadInstance *thread, EE::decode_t &insn) { return helper_" << isa.ISAName << "_instruction_predicate(ee, thread, insn); }"; 
	}
	
	
	str << "template<bool trace=false> archsim::ExecutionResult StepInstruction_" << isa.ISAName << "(EE *ee, archsim::ThreadInstance *thread, EE::decode_t &decode) {";
	str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	str << "archsim::ExecutionResult interp_result;";
	str << "bool should_execute = true;";
	
	str << "if(trace) { if(thread->GetTraceSource() == nullptr) { throw std::logic_error(\"\"); } thread->GetTraceSource()->Trace_Insn(thread->GetPC().Get(), decode.ir, false, thread->GetModeID(), thread->GetExecutionRing(), 1); }";
	// check predicate
	if(has_is_predicated && has_instruction_predicate) {
		
		str << "should_execute =  !" << isa.ISAName << "_is_predicated(ee, thread, decode) || " << isa.ISAName << "_check_predicate(ee, thread, decode);";
	}	
	str << "if(should_execute) {";
	str << " switch(decode.Instr_Code) {";
	str << " using namespace gensim::" << Manager.GetArch().Name << ";";
	
	for(auto i : isa.Instructions) {
		str << "case INST_" << isa.ISAName << "_" << i.second->Name << ": interp_result = StepInstruction_" << isa.ISAName << "_" << i.first << "<trace>(ee, thread, decode); break;";
	}
	
	str << " default: LC_ERROR(LogExecutionEngine) << \"Unknown instruction\"; return archsim::ExecutionResult::Abort;";
	
	str << "}";
	str << "}";
	
	str << "if(!decode.GetEndOfBlock() || !should_execute) {";
	str << "  interface.write_pc(interface.read_pc() + decode.Instr_Length);";
	str << "}";
	
	str << "if(trace) { thread->GetTraceSource()->Trace_End_Insn(); }";
	str << "return interp_result;";
	str << "}";
	
	return true;
}

bool InterpEEGenerator::GenerateBehavioursDescriptors(util::cppformatstream& str) const
{
	std::string isa_descriptors;
	for(auto i : Manager.GetArch().ISAs) {
		if(!isa_descriptors.empty()) {
			isa_descriptors += ", ";
		}
		isa_descriptors += "behaviours_" + i->ISAName;
		
		std::string behaviours_list;
		
		for(auto action : i->GetSSAContext().Actions()) {
			if(action.second->GetPrototype().HasAttribute(gensim::genc::ActionAttribute::Export)) {
				if(!behaviours_list.empty()) {
					behaviours_list += ", ";
				}
				behaviours_list += "bd_" + i->ISAName + "_" + action.first;
				
				str << "static archsim::BehaviourDescriptor bd_" << i->ISAName << "_" << action.first << "(\"" << action.first << "\", [](const archsim::InvocationContext &ctx){ helper_" << i->ISAName << "_" << action.first << "<false>(nullptr, ctx.GetThread()";
				
				// unpack arguments
				for(int index = 0; index < action.second->GetPrototype().ParameterTypes().size(); ++index) {
					auto &argtype = action.second->GetPrototype().ParameterTypes().at(index);
					// if we're accessing a struct, assume it's a decode_t
					std::string type_string;
					
					if(argtype.Reference) {
						UNIMPLEMENTED;
					}
					
					if(argtype.IsStruct()) {
						UNIMPLEMENTED;
					}
						
					str << ", (" << argtype.GetCType() << ")ctx.GetArg(" << index << ")";
					
				}
				
				str << "); return 0; });";
			}
		}
		
		str << "static archsim::ISABehavioursDescriptor behaviours_" << i->ISAName << "(\"" << i->ISAName << "\", {" << behaviours_list << "});";
	}
	
	str << "namespace " << Manager.GetArch().Name << "{";
	str << "archsim::BehavioursDescriptor behaviours ({" << isa_descriptors << "});";
	str << "}";
	
	return true;
}


InterpEEGenerator::~InterpEEGenerator()
{

}


DEFINE_COMPONENT(InterpEEGenerator, ee_interp)