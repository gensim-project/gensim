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
		"#include <interpret/Interpreter.h>\n"
		"#include <cstdint>\n"
		
		"namespace gensim {"
		"namespace " << Manager.GetArch().Name << "{"
		
		"class Interpreter : public archsim::interpret::Interpreter {"
		"public:"
		"   virtual archsim::core::execution::ExecutionResult StepBlock(archsim::core::thread::ThreadInstance *thread);"
		"	using decode_t = gensim::" << Manager.GetArch().Name << "::Decode;"		
		"private:"
		"	gensim::DecodeContext *decode_context_;"
		"  uint32_t DecodeInstruction(archsim::core::thread::ThreadInstance *thread, decode_t &inst);"
		"  archsim::core::execution::ExecutionResult StepInstruction(archsim::core::thread::ThreadInstance *thread, decode_t &inst);"
		
		"};"
		""
		;

	str << "}";
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
		"#include <gensim/gensim_decode_context.h>\n"
		"#include <cmath>\n"
		;
	
	str << "using namespace gensim::" << Manager.GetArch().Name << ";";
	
	GenerateDecodeInstruction(str);
	
	str << "archsim::core::execution::ExecutionResult Interpreter::StepBlock(archsim::core::thread::ThreadInstance *thread) { ";
	GenerateBlockExecutor(str);
	str << "}";
	
	GenerateHelperFunctions(str);
	GenerateStepInstruction(str);
	
	GenerateBehavioursDescriptors(str);
	
	return true;
}

static void GenerateHelperFunctionPrototype(gensim::util::cppformatstream &str, const gensim::isa::ISADescription &isa, const gensim::genc::ssa::SSAFormAction *action) 
{
	str << "template<bool trace=false> " << action->GetPrototype().ReturnType().GetCType() << " helper_" << isa.ISAName << "_" << action->GetPrototype().GetIRSignature().GetName() << "(archsim::core::thread::ThreadInstance *thread";

	for(auto i : action->ParamSymbols) {
		// if we're accessing a struct, assume that it's an instruction
		if(i->GetType().IsStruct()) {
			str << ", Interpreter::decode_t &inst";
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
	str << "uint32_t Interpreter::DecodeInstruction(archsim::core::thread::ThreadInstance *thread, Interpreter::decode_t &inst) {";
	str << "  if(decode_context_ == nullptr) { decode_context_ = thread->GetEmulationModel().GetNewDecodeContext(*thread); }";
	str << "  gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	str << "  auto result = decode_context_->DecodeSync(archsim::Address(interface.read_pc()), thread->GetModeID(), inst);";
	str << "  return result;";
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
		"  if(dcode_exception) { thread->TakeMemoryException(thread->GetFetchMI(), thread->GetPC()); return archsim::core::execution::ExecutionResult::Exception; }"
		"  if(archsim::options::InstructionTick) { thread->GetPubsub().Publish(PubSubType::InstructionExecute, nullptr); } "
		"  auto result = StepInstruction(thread, inst);"
		"  if(inst.GetEndOfBlock()) { return archsim::core::execution::ExecutionResult::Continue; }"
		"  if(result != archsim::core::execution::ExecutionResult::Continue) { return result; }"
		"}";
	
	return true;
}

bool InterpEEGenerator::GenerateStepInstruction(util::cppformatstream& str) const
{
	for(auto i : Manager.GetArch().ISAs) {
		GenerateStepInstructionISA(str, *i);
	}
	
	str << 
		"archsim::core::execution::ExecutionResult Interpreter::StepInstruction(archsim::core::thread::ThreadInstance *thread, Interpreter::decode_t &inst) {";
	str << "thread->GetMetrics().InstructionCount++;";
	str << "switch(thread->GetModeID()) {";
	
	for(auto i : Manager.GetArch().ISAs) {
		str << "case " << i->isa_mode_id << ": if(archsim::options::Trace) { return StepInstruction_" << i->ISAName << "<true>(thread, inst); } else { return StepInstruction_" << i->ISAName << "<false>(thread, inst); }";
	}
		
	str << 
		"  default: "
		"    LC_ERROR(LogInterpreter) << \"Unknown mode\"; break;"
		"  }";
	
	str << 
		"  return archsim::core::execution::ExecutionResult::Abort; "
		"}";
	
	return true;
}

bool InterpEEGenerator::GenerateStepInstructionInsn(util::cppformatstream& str, isa::InstructionDescription& insn) const
{
	str << "template<bool trace=false> archsim::core::execution::ExecutionResult StepInstruction_" << insn.ISA.ISAName << "_" << insn.Name << "(archsim::core::thread::ThreadInstance *thread, Interpreter::decode_t &inst) {";
	str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	
	gensim::generator::GenCInterpreterGenerator gci (Manager);
	gci.GenerateExecuteBodyFor(str, *static_cast<const gensim::genc::ssa::SSAFormAction*>(insn.ISA.GetSSAContext().GetAction(insn.BehaviourName)));
	
	str << "return archsim::core::execution::ExecutionResult::Continue;";
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
		str << "bool " << isa.ISAName << "_is_predicated(archsim::core::thread::ThreadInstance *thread, Interpreter::decode_t &insn) { return helper_" << isa.ISAName << "_instruction_is_predicated(thread, insn); }";
	}
	if(has_instruction_predicate) {
		str << "bool " << isa.ISAName << "_check_predicate(archsim::core::thread::ThreadInstance *thread, Interpreter::decode_t &insn) { return helper_" << isa.ISAName << "_instruction_predicate(thread, insn); }"; 
	}
	
	
	str << "template<bool trace=false> archsim::core::execution::ExecutionResult StepInstruction_" << isa.ISAName << "(archsim::core::thread::ThreadInstance *thread, Interpreter::decode_t &decode) {";
	str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
	str << "archsim::core::execution::ExecutionResult interp_result = archsim::core::execution::ExecutionResult::Continue;";
	str << "bool should_execute = true;";
	
	str << "if(trace) { if(thread->GetTraceSource() == nullptr) { throw std::logic_error(\"\"); } thread->GetTraceSource()->Trace_Insn(thread->GetPC().Get(), decode.ir, false, thread->GetModeID(), thread->GetExecutionRing(), 1); }";
	// check predicate
	if(has_is_predicated && has_instruction_predicate) {
		
		str << "should_execute =  !" << isa.ISAName << "_is_predicated(thread, decode) || " << isa.ISAName << "_check_predicate(thread, decode);";
	}	
	str << "if(should_execute) {";
	str << " switch(decode.Instr_Code) {";
	str << " using namespace gensim::" << Manager.GetArch().Name << ";";
	
	for(auto i : isa.Instructions) {
		str << "case INST_" << isa.ISAName << "_" << i.second->Name << ": interp_result = StepInstruction_" << isa.ISAName << "_" << i.first << "<trace>(thread, decode); break;";
	}
	
	str << " default: LC_ERROR(LogInterpreter) << \"Unknown instruction\"; return archsim::core::execution::ExecutionResult::Abort;";
	
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
				behaviours_list += "bd_" + i->ISAName + "_" + action.first + "()";
				
				str << "static archsim::BehaviourDescriptor bd_" << i->ISAName << "_" << action.first << "() { archsim::BehaviourDescriptor bd (\"" << action.first << "\", [](const archsim::InvocationContext &ctx){ helper_" << i->ISAName << "_" << action.first << "<false>(ctx.GetThread()";
				
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
				
				str << "); return 0; }); return bd; }";
			}
		}
	
		str << "namespace " << Manager.GetArch().Name << "{";
		str << "archsim::ISABehavioursDescriptor get_behaviours_" << i->ISAName << "() { static archsim::ISABehavioursDescriptor bd ({" << behaviours_list << "}); return bd; }";
		str << "}";
	}
	
	
	return true;
}


InterpEEGenerator::~InterpEEGenerator()
{

}


DEFINE_COMPONENT(InterpEEGenerator, ee_interp)
