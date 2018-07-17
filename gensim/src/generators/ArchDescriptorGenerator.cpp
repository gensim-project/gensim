/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/analysis/CallGraphAnalysis.h"
#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/ArchDescriptorGenerator.h"
#include "generators/GenCInterpreter/GenCInterpreterGenerator.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSATypeFormatter.h"

using namespace gensim::generator;

ArchDescriptorGenerator::ArchDescriptorGenerator(GenerationManager& manager) : GenerationComponent(manager, "ArchDescriptor")
{
	manager.AddModuleEntry(ModuleEntry("ArchDescriptor", "gensim::" + manager.GetArch().Name + "::ArchDescriptor", "arch.h", ModuleEntryType::ArchDescriptor));
}

bool ArchDescriptorGenerator::Generate() const
{
	util::cppformatstream header, source;
	GenerateHeader(header);
	GenerateSource(source);

	std::ofstream hfile ((Manager.GetTarget() + "/arch.h").c_str());
	hfile << header.str();
	std::ofstream sfile ((Manager.GetTarget() + "/arch.cpp").c_str());
	sfile << source.str();

	return true;
}

std::string ArchDescriptorGenerator::GetFunction() const
{
	return "ArchDescriptor";
}

const std::vector<std::string> ArchDescriptorGenerator::GetSources() const
{
	return {"arch.cpp"};
}

static void GenerateHelperFunctionPrototype(gensim::util::cppformatstream &str, const gensim::isa::ISADescription &isa, const gensim::genc::ssa::SSAFormAction *action, bool addTemplateDefaultValue)
{
	gensim::genc::ssa::SSATypeFormatter formatter;
	formatter.SetStructPrefix("gensim::" + action->Arch->Name + "::decode_t::");

	if(addTemplateDefaultValue)
		str << "template<bool trace=false> ";
	else
		str << "template<bool trace> ";

	str << formatter.FormatType(action->GetPrototype().ReturnType()) << " helper_" << isa.ISAName << "_" << action->GetPrototype().GetIRSignature().GetName() << "(archsim::core::thread::ThreadInstance *thread";

	for(auto i : action->ParamSymbols) {
		str << ", " << formatter.FormatType(i->GetType()) << " " << i->GetName();
	}
	str << ")";
}

bool ArchDescriptorGenerator::GenerateSource(util::cppformatstream &str) const
{
	str << "#include <core/thread/ThreadInstance.h>\n";
	str << "#include <core/execution/ExecutionEngine.h>\n";
	str << "#include \"arch.h\"\n";

	if(Manager.GetComponent(GenerationManager::FnDecode)) {
		str << "#include \"decode.h\"\n";
	}

	str << "#include \"jumpinfo.h\"\n";

	str << "using namespace gensim::" << Manager.GetArch().Name << ";";

	auto rfile = Manager.GetArch().GetRegFile();
	std::string entry_list;
	for(auto i : rfile.GetBanks()) {
		str << "static archsim::RegisterFileEntryDescriptor rfd_entry_" << i->ID << "(\"" << i->ID <<"\", " << i->GetIndex() << ", " << i->GetRegFileOffset() << ", " << i->GetRegisterCount() << ", " << i->GetRegisterWidth() << ", " << i->GetRegisterStride() << ");";
		if(entry_list.size()) {
			entry_list += ", ";
		}
		entry_list += "rfd_entry_" + i->ID;
	}

	for(auto i : rfile.GetSlots()) {
		std::string tag = "";
		if(i->HasTag()) {
			tag = i->GetTag();
		}
		str << "static archsim::RegisterFileEntryDescriptor rfd_entry_" << i->GetID() << "(\"" << i->GetID() <<"\", " << i->GetIndex() << ", " << i->GetRegFileOffset() << ", " << 1 << ", " << i->GetWidth() << ", " << i->GetWidth() << ", \"" << tag << "\");";
		if(entry_list.size()) {
			entry_list += ", ";
		}
		entry_list += "rfd_entry_" + i->GetID();
	}

	str << "static archsim::RegisterFileDescriptor rfd(" << rfile.GetSize() << ", {" << entry_list << "});";

	std::string mem_interface_list;
	for(const auto &mem_interface : Manager.GetArch().GetMemoryInterfaces().GetInterfaces()) {
		str << "static archsim::MemoryInterfaceDescriptor mem_" << mem_interface.second.GetName() << "(\"" << mem_interface.second.GetName() << "\", " << mem_interface.second.GetAddressWidthInBytes() << ", " << mem_interface.second.GetDataWidthInBytes() << ", " << (uint64_t)mem_interface.second.IsBigEndian() << ", " << mem_interface.second.GetID() << ");";

		if(mem_interface_list.size()) {
			mem_interface_list += ", ";
		}
		mem_interface_list += "mem_" + mem_interface.second.GetName();
	}

	str << "static archsim::MemoryInterfacesDescriptor misd ({" << mem_interface_list << "},\"" << Manager.GetArch().GetMemoryInterfaces().GetFetchInterface().GetName() << "\");";

	std::string feature_list;
	for(const auto &feature : Manager.GetArch().GetFeatures()) {
		if(!feature_list.empty()) {
			feature_list += ", ";
		}
		feature_list += "feature_" + feature.GetName();
		str << "static archsim::FeatureDescriptor feature_" << feature.GetName() << "(\"" << feature.GetName() << "\", " << feature.GetId() << ", " << feature.GetDefaultLevel() << ");";
	}
	str << "static archsim::FeaturesDescriptor features ({" << feature_list << "});";

	std::string isa_list;
	for(const auto &isa : Manager.GetArch().ISAs) {
		if(!isa_list.empty()) {
			isa_list += ", ";
		}
		isa_list += "isa_" + isa->ISAName;


		std::string behaviours_list = "";
		GenCInterpreterGenerator interp (Manager);

		// figure out the complete list of behaviours we need to output in order to service this ISA's exported behaviours
		gensim::genc::ssa::CallGraphAnalysis cga;
		auto callgraph = cga.Analyse(isa->GetSSAContext());
		std::set<gensim::genc::ssa::SSAActionBase *> exported_actions;
		for(const auto &action : isa->GetSSAContext().Actions()) {
			if(action.second->GetPrototype().HasAttribute(gensim::genc::ActionAttribute::Export)) {
				auto allcallees = callgraph.GetDeepCallees((gensim::genc::ssa::SSAFormAction*)action.second);
				exported_actions.insert(allcallees.begin(), allcallees.end());
				exported_actions.insert(action.second);
			}
		}

		for(auto action : exported_actions) {
			GenerateHelperFunctionPrototype(str, *isa, (gensim::genc::ssa::SSAFormAction*)action, true);
			str << ";";
		}

		for(const auto &action : exported_actions) {

			// generate helper function
			GenerateHelperFunctionPrototype(str, *isa, (gensim::genc::ssa::SSAFormAction*)action, false);
			str << "{";
			str << "gensim::" << Manager.GetArch().Name << "::ArchInterface interface(thread);";
			interp.GenerateExecuteBodyFor(str, *(gensim::genc::ssa::SSAFormAction*)action);
			str << "}";

			if(action->GetPrototype().HasAttribute(gensim::genc::ActionAttribute::Export)) {
				if(!behaviours_list.empty()) {
					behaviours_list += ", ";
				}
				behaviours_list += "bd_" + isa->ISAName + "_" + action->GetName() + "()";

				str << "static archsim::BehaviourDescriptor bd_" << isa->ISAName << "_" << action->GetName() << "() { archsim::BehaviourDescriptor bd (\"" << action->GetPrototype().GetIRSignature().GetName() << "\", [](const archsim::InvocationContext &ctx){ helper_" << isa->ISAName << "_" << action->GetPrototype().GetIRSignature().GetName() << "<false>(ctx.GetThread()";

				// unpack arguments
				for(size_t index = 0; index < action->GetPrototype().ParameterTypes().size(); ++index) {
					auto &argtype = action->GetPrototype().ParameterTypes().at(index);
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

		str << "static archsim::ISABehavioursDescriptor get_behaviours_" << isa->ISAName << "() { return archsim::ISABehavioursDescriptor({" << behaviours_list << "}); };";

		str << "static auto " << isa->ISAName << "_decode_instr = [](archsim::Address addr, archsim::MemoryInterface *interface, gensim::BaseDecode &decode){ return ((gensim::" << Manager.GetArch().Name << "::Decode&)decode).DecodeInstr(addr, " << (uint32_t)isa->isa_mode_id << ", *interface); };";
		str << "static auto " << isa->ISAName << "_newdecoder = []() { return new gensim::" << Manager.GetArch().Name << "::Decode(); };";
		str << "static auto " << isa->ISAName << "_newjumpinfo = []() { return new gensim::" << Manager.GetArch().Name << "::JumpInfoProvider(); };";
		str << "static auto " << isa->ISAName << "_newdtc = []() { return nullptr; };";

		str << "static archsim::ISADescriptor isa_" << isa->ISAName << " (\"" << isa->ISAName << "\", " << (uint32_t)isa->isa_mode_id << ", " << isa->ISAName << "_decode_instr, " << isa->ISAName << "_newdecoder, " << isa->ISAName << "_newjumpinfo, " << isa->ISAName << "_newdtc, get_behaviours_" << isa->ISAName << "());";

	}

	str << "ArchDescriptor::ArchDescriptor() : archsim::ArchDescriptor(\"" << Manager.GetArch().Name << "\", rfd, misd, features, {" << isa_list << "}) {}";

	return true;
}

bool ArchDescriptorGenerator::GenerateHeader(util::cppformatstream &str) const
{
	str << "#ifndef ARCH_DESC_H\n";
	str << "#define ARCH_DESC_H\n";
	str << "#include <core/arch/ArchDescriptor.h>\n";
	str << "#include <core/thread/ThreadInstance.h>\n";
	str << "#include <gensim/gensim_processor_api.h>\n";
	str << "#include <util/Vector.h>\n";

	str << "#include <util/int128.h>\n";
	str << "using uint128_t = wutils::int128<false>;";
	str << "using int128_t = wutils::int128<true>;";

	str << "namespace gensim {";
	str << "namespace " << Manager.GetArch().Name << " {";
	str << "class ArchDescriptor : public archsim::ArchDescriptor {";
	str << "public: ArchDescriptor();";
	str << "};";

	GenerateThreadInterface(str);


	str << "} }"; // namespace

	str << "\n#endif\n";

	return true;
}

bool ArchDescriptorGenerator::GenerateThreadInterface(util::cppformatstream &str) const
{
	str << "class ArchInterface {";
	str << "public: ArchInterface(archsim::core::thread::ThreadInstance *thread) : thread_(thread), reg_file_((char*)thread->GetRegisterFile()) {}";

	for(gensim::arch::RegBankViewDescriptor *rbank : Manager.GetArch().GetRegFile().GetBanks()) {
		std::string read_trace_string = "";
		std::string write_trace_string = "";
		if(rbank->GetElementSize() > 8) {
			read_trace_string = "";
			write_trace_string = "";
		} else if(rbank->GetRegisterIRType().VectorWidth > 1) {
			read_trace_string = "if(trace) { thread_->GetTraceSource()->Trace_Bank_Reg_Read(true, " + std::to_string(rbank->GetIndex()) + ", idx, (char*)value.data(), " + std::to_string(rbank->GetRegisterWidth()) + "); }";
			write_trace_string = "if(trace) { thread_->GetTraceSource()->Trace_Bank_Reg_Write(true, " + std::to_string(rbank->GetIndex()) + ", idx, (char*)value.data(), " + std::to_string(rbank->GetRegisterWidth()) + "); }";
		} else {
			read_trace_string = "if(trace) { thread_->GetTraceSource()->Trace_Bank_Reg_Read(true, " + std::to_string(rbank->GetIndex()) + ", idx, (" + rbank->GetElementIRType().GetCType() + ")value); }";
			write_trace_string = "if(trace) { thread_->GetTraceSource()->Trace_Bank_Reg_Write(true, " + std::to_string(rbank->GetIndex()) + ", idx, (" + rbank->GetElementIRType().GetCType() + ") value); }";
		}
		str << "template<bool trace=false> " << rbank->GetRegisterIRType().GetCType() << " read_register_bank_" << rbank->ID << "(uint32_t idx) const { auto value = *(" << rbank->GetRegisterIRType().GetCType() << "*)(reg_file_ + " << rbank->GetRegFileOffset() << " + (idx * " << rbank->GetRegisterStride() << ")); " << read_trace_string << " return value; }";
		str << "template<bool trace=false> void write_register_bank_" << rbank->ID << "(uint32_t idx, " << rbank->GetRegisterIRType().GetCType() << " value) { *(" << rbank->GetRegisterIRType().GetCType() << "*)(reg_file_ + " << rbank->GetRegFileOffset() << " + (idx * " << rbank->GetRegisterStride() << ")) = value; " << write_trace_string << " }";
	}
	for(gensim::arch::RegSlotViewDescriptor *slot : Manager.GetArch().GetRegFile().GetSlots()) {
		str << "template<bool trace=false> " << slot->GetIRType().GetCType() << " read_register_" << slot->GetID() << "() const { auto value = *(" << slot->GetIRType().GetCType() << "*)(reg_file_ + " << slot->GetRegFileOffset() << "); if(trace) { thread_->GetTraceSource()->Trace_Reg_Read(1, " << slot->GetIndex() << ", value); } return value; }";
		str << "template<bool trace=false> " << "void write_register_" << slot->GetID() << "(" << slot->GetIRType().GetCType() << " value) { *(" << slot->GetIRType().GetCType() << "*)(reg_file_ + " << slot->GetRegFileOffset() << ") = value; if(trace) { thread_->GetTraceSource()->Trace_Reg_Write(1, " << slot->GetIndex() << ", value); } }";
	}

	// read/write pc
	auto pc_descriptor = Manager.GetArch().GetRegFile().GetTaggedRegSlot("PC");

	str << "archsim::Address read_pc() const {";
	str << "return archsim::Address(read_register_" << pc_descriptor->GetID() << "());";
	str << "}";

	str << "void write_pc(archsim::Address new_pc) {";
	str << "write_register_" << pc_descriptor->GetID() << "(new_pc.Get());";
	str << "}";

	str << "private: "
	    "archsim::core::thread::ThreadInstance *thread_;"
	    "char *reg_file_;";
	str << "};";

	return true;
}



DEFINE_COMPONENT(ArchDescriptorGenerator, arch);
