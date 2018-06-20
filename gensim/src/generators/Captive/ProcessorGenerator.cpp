/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/GenerationManager.h"
#include "arch/ArchDescription.h"
#include "arch/RegisterFile.h"
#include "isa/ISADescription.h"

using namespace gensim::generator;

namespace gensim
{
	namespace generator
	{
		namespace captive
		{

			class CpuGenerator : public GenerationComponent
			{
			public:

				CpuGenerator(GenerationManager &man) : GenerationComponent(man, "captive_cpu")
				{
				}

				virtual ~CpuGenerator()
				{
				}

				std::string GetFunction() const override
				{
					return "cpu";
				}

				const std::vector<std::string> GetSources() const override
				{
					return std::vector<std::string>();
				}

				bool Generate() const override
				{
					bool success = true;
					util::cppformatstream header_str;
					util::cppformatstream source_str;

					// generate the source streams
					success &= GenerateCPUHeader(header_str);
					success &= GenerateCPUSource(source_str);

					if (success) {
						WriteOutputFile(Manager.GetArch().Name + "-cpu.h", header_str);
						WriteOutputFile(Manager.GetArch().Name + "-cpu.cpp", source_str);
					}

					return success;
				}

			private:

				inline std::string ClassNameForDecoder() const
				{
					return Manager.GetArch().Name + "_decode";
				}

				inline std::string ClassNameForCPU() const
				{
					return Manager.GetArch().Name + "_cpu";
				}

				inline std::string EnumNameForFeatures() const
				{
					return Manager.GetArch().Name + "_features";
				}

				bool GenerateCPUHeader(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#pragma once\n";

					str << "#include <define.h>\n";
					str << "#include <cpu.h>\n";

					str << "namespace captive {";
					str << "namespace arch {";
					str << "namespace " << arch.Name << " {";

					str << "class " << arch.Name << "_environment;";

					// Features
					str << "enum " << EnumNameForFeatures() << " {\n";

					for (const auto& feat : arch.GetFeatures()) {
						str << feat.GetName() << " = " << feat.GetId() << ",\n";
					}

					str << "};";

					// CPU
					str << "class " << ClassNameForCPU() << " : public CPU {";
					str << "friend class " << arch.Name << "_mmu;";

					str << "public:\n";

					str << ClassNameForCPU() << "(" << arch.Name << "_environment& env, PerCPUData *per_cpu_data);";
					str << "virtual ~" << arch.Name << "_cpu();";

					str << "bool init() override;";

					str << "void dump_state(bool show_hidden) const override;";

					str << "bool handle_irq(uint32_t isr) override;";
					str << "bool handle_mmu_fault(gva_t va, const captive::arch::mmu::AddressTranslationContext& ctx) override;";
					str << "bool handle_single_step() override;";
					str << "void handle_undefined_instruction() override;";

					str << "bool interrupts_enabled(uint8_t irq_line) const override;";

					str << "struct reg_state_t { \n";

					for (const auto& space : arch.GetRegFile().GetSpaces()) {
						str << "uint8_t space_" << space->GetOffsetInFile() << "[" << space->GetSize() << "];";
					}

					str << "} packed regs;";

					str << "struct reg_off_t { \n";

					for (const auto& bank : arch.GetRegFile().GetBanks()) {
						switch (bank->GetRegisterWidth()) {
							case 1:
								str << "uint8_t";
								break;
							case 2:
								str << "uint16_t";
								break;
							case 4:
								str << "uint32_t";
								break;
							case 8:
								str << "uint64_t";
								break;
							case 16:
								str << "uint128_t";
								break;
							default:
								str << "void";
								break;
						}

						str << " *" << bank->ID << ";";
					}

					for (const auto& slot : arch.GetRegFile().GetSlots()) {
						switch (slot->GetWidth()) {
							case 1:
								str << "uint8_t";
								break;
							case 2:
								str << "uint16_t";
								break;
							case 4:
								str << "uint32_t";
								break;
							case 8:
								str << "uint64_t";
								break;
						}

						str << " *" << slot->GetID() << ";";
					}

					str << "} reg_offsets;";

					str << "protected:\n";
					str << "bool decode_instruction_virt(uint8_t isa, gva_t addr, Decode *insn) override;";
					str << "bool decode_instruction_phys(uint8_t isa, gpa_t addr, Decode *insn) override;";
					str << "JumpInfo get_instruction_jump_info(const Decode *insn) override;";

					str << "void __builtin_set_feature(" << EnumNameForFeatures() << " feature, uint32_t value) { feature_manager().set_feature(feature, value); }";
					str << "uint32_t __builtin_get_feature(" << EnumNameForFeatures() << " feature) const { return feature_manager().get_feature(feature); }";

					str << "public:\n";

					for (const auto& helper : arch.ISAs.front()->HelperFns) {

						fprintf(stderr, "HELPER: %s\n", helper.name.c_str());
						for (const auto& attr : helper.attributes) {
							fprintf(stderr, "ATTR: %s\n", attr.c_str());
						}

						if (helper.HasAttr("global") || helper.HasAttr("noinline")) str << helper.GetPrototype() << ";";
					}

					str << "};";

					str << "}";
					str << "}";
					str << "}";

					return true;
				}

				bool GenerateCPUSource(util::cppformatstream& str) const
				{
					const arch::ArchDescription &arch = Manager.GetArch();

					str << "#include <" << arch.Name << "-cpu.h>\n";
					str << "#include <" << arch.Name << "-env.h>\n";
					str << "#include <" << arch.Name << "-decode.h>\n";
					str << "#include <" << arch.Name << "-disasm.h>\n";
					str << "#include <" << arch.Name << "-jit2.h>\n";
					str << "#include <" << arch.Name << "-mmu.h>\n";

					str << "using namespace captive::arch::" << arch.Name << ";";

					str << ClassNameForCPU() << "::" << arch.Name << "_cpu(" << arch.Name << "_environment& env, PerCPUData *per_cpu_data) : CPU(env, per_cpu_data)";
					str << "{";

					str << "uint8_t *reg_file = (uint8_t *)_reg_state;";

					for (const auto& bank : arch.GetRegFile().GetBanks()) {
						str << "reg_offsets." << bank->ID << " = (";

						switch (bank->GetRegisterWidth()) {
							case 1:
								str << "uint8_t";
								break;
							case 2:
								str << "uint16_t";
								break;
							case 4:
								str << "uint32_t";
								break;
							case 8:
								str << "uint64_t";
								break;
							case 16:
								str << "uint128_t";
								break;
							default:
								str << "void";
								break;
						}

						str << " *)&reg_file[" << bank->GetRegFileOffset() << "];";

						for (int i = 0; i < bank->GetRegisterCount(); i++) {
							uint32_t offset = bank->GetRegFileOffset() + (i * bank->GetRegisterStride());
							str << "add_reg_name(" << offset << ", \"" << bank->ID << "(" << i << ")\");";
						}
					}

					for (const auto& slot : arch.GetRegFile().GetSlots()) {
						str << "reg_offsets." << slot->GetID() << " = (";
						switch (slot->GetWidth()) {
							case 1:
								str << "uint8_t";
								break;
							case 2:
								str << "uint16_t";
								break;
							case 4:
								str << "uint32_t";
								break;
							case 8:
								str << "uint64_t";
								break;
						}

						str << " *)&reg_file[" << slot->GetRegFileOffset() << "];\n";

						if (slot->HasTag()) {
							str << "tagged_reg_offsets." << slot->GetTag() << " = reg_offsets." << slot->GetID() << ";";
						}

						str << "add_reg_name(" << slot->GetRegFileOffset() << ", \"" << slot->GetID() << "\");";
					}

					str << "tagged_reg_offsets.base = (void *)reg_file;";

					str << "}";

					str << ClassNameForCPU() << "::~" << arch.Name << "_cpu() { }";

					str << "void " << ClassNameForCPU() << "::dump_state(bool show_hidden) const";
					str << "{";

					str << "printf(\"*** CPU STATE\\n\");";

					for (const auto& bank : arch.GetRegFile().GetBanks()) {
						if (bank->Hidden) {
							str << "if (show_hidden) {";
						}

						str << "printf(\"Register Bank " << bank->ID << ":\\n\");";

						for (unsigned int i = 0; i < bank->GetRegisterCount(); i++) {
							str << "printf(\"  r%02d = %016x\\n\", " << i << ", reg_offsets." << bank->ID << "[" << i << "]);";
						}

						if (bank->Hidden) {
							str << "}";
						}
					}

					for (const auto& slot : arch.GetRegFile().GetSlots()) {
						str << "printf(\"" << slot->GetID() << " = %016x\\n\", *reg_offsets." << slot->GetID() << ");";
					}

					str << "}";

					str << "bool " << ClassNameForCPU() << "::init()";
					str << "{";

					str << "_trace = new Trace(*new " << arch.Name << "_disasm());";
					str << "_dbt = new " << arch.Name << "_jit2<false>();";
					str << "_tracing_dbt = new " << arch.Name << "_jit2<true>();";
					str << "_mmu = " << arch.Name << "_mmu::create(*this);";

					/*str << "jit_state.registers = &regs;";
					str << "jit_state.registers_size = sizeof(regs);";

					str << "bzero(&regs, sizeof(regs));";*/

					str << "return true;";

					str << "}";

					str << "bool " << ClassNameForCPU() << "::decode_instruction_virt(uint8_t isa, gva_t addr, Decode* insn)";
					str << "{";
					str << "switch (isa) {";

					for (auto isa : arch.ISAs) {
						str << "case " << (uint32_t) isa->isa_mode_id << ":\n";
						str << "return ((" << ClassNameForDecoder() << " *)insn)->decode(" << ClassNameForDecoder() << "::" << arch.Name << "_" << isa->ISAName << ", addr, (uint8_t *)(uint64_t)addr);";
					}

					str << "default:\n";
					str << "return false;";
					str << "}";
					str << "}";

					str << "bool " << ClassNameForCPU() << "::decode_instruction_phys(uint8_t isa, gpa_t addr, Decode* insn)";
					str << "{";
					str << "switch (isa) {";

					for (auto isa : arch.ISAs) {
						str << "case " << (uint32_t) isa->isa_mode_id << ":\n";
						str << "return ((" << ClassNameForDecoder() << " *)insn)->decode(" << ClassNameForDecoder() << "::" << arch.Name << "_" << isa->ISAName << ", addr, (uint8_t *)(guest_phys_to_vm_virt((uint64_t)addr)));";
					}

					str << "default:\n";
					str << "return false;";
					str << "}";
					str << "}";

					str << "captive::arch::JumpInfo " << ClassNameForCPU() << "::get_instruction_jump_info(const Decode *insn)";
					str << "{";
					str << "return " << ClassNameForDecoder() << "::get_jump_info((const " << ClassNameForDecoder() << " *)insn);";
					str << "}";

					// Behaviours
					str << "#define read_register(__REG) (*reg_offsets.__REG)\n";
					str << "#define read_register_bank(__RB, __REG) (reg_offsets.__RB[__REG])\n";
					str << "#define write_register(__REG, __VAL) (*reg_offsets.__REG) = (__VAL)\n";
					str << "#define write_register_bank(__RB, __REG, __VAL) reg_offsets.__RB[__REG] = (__VAL)\n\n";

					str << "typedef uint32_t uint32;";
					str << "typedef uint16_t uint16;";
					str << "typedef uint8_t uint8;";
					str << "typedef int32_t sint32;";
					str << "typedef int16_t sint16;";
					str << "typedef int8_t sint8;";

					for (const auto& helper : arch.ISAs.front()->HelperFns) {
						if (helper.HasAttr("global") || helper.HasAttr("noinline")) {
							str << helper.GetPrototype(ClassNameForCPU()) << "{ " << helper.body << " }";
						}
					}

					str << "bool " << ClassNameForCPU() << "::handle_irq(uint32_t isr)";
					str << "{";
					str << *(arch.ISAs.front()->BehaviourActions.at("irq"));
					str << "}";

					str << "bool " << ClassNameForCPU() << "::handle_mmu_fault(gva_t va, const captive::arch::mmu::AddressTranslationContext& ctx)";
					str << "{";
					str << *(arch.ISAs.front()->BehaviourActions.at("mmu_fault"));
					str << "}";

					str << "bool " << ClassNameForCPU() << "::handle_single_step()";
					str << "{";
					str << *(arch.ISAs.front()->BehaviourActions.at("single_step"));
					str << "}";

					/*str << "bool " << ClassNameForCPU() << "::handle_data_fault(MMU::resolution_fault fault)";
					str << "{";
					str << *(arch.ISAs.front()->BehaviourActions.at("data_fault"));
					str << "}";

					str << "bool " << ClassNameForCPU() << "::handle_fetch_fault(MMU::resolution_fault fault)";
					str << "{";
					str << *(arch.ISAs.front()->BehaviourActions.at("fetch_fault"));
					str << "}";*/

					str << "void " << ClassNameForCPU() << "::handle_undefined_instruction()";
					str << "{";
					str << *(arch.ISAs.front()->BehaviourActions.at("undef"));
					str << "}";

					str << "bool " << ClassNameForCPU() << "::interrupts_enabled(uint8_t irq_line) const";
					str << "{";
					str << "return false;";
					str << "}";

					return true;
				}
			};

		}
	}
}

DEFINE_COMPONENT(gensim::generator::captive::CpuGenerator, captive_cpu)
//COMPONENT_INHERITS(captive_cpu, base_decode);
