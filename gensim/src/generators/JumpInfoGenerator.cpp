/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/GenerationManager.h"
#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "Util.h"

using namespace gensim;
using namespace gensim::generator;

class JumpInfoGenerator : public GenerationComponent
{
public:
	JumpInfoGenerator(GenerationManager &man) : GenerationComponent(man, "jumpinfo") {}
	virtual ~JumpInfoGenerator() {}

	bool Generate() const override;
	std::string GetFunction() const override
	{
		return GenerationManager::FnJumpInfo;
	}
	const std::vector<std::string> GetSources() const override
	{
		return { "jumpinfo.cpp" };
	}

private:
	bool GenerateHeader(util::cppformatstream &stream) const;
	bool GenerateSource(util::cppformatstream &stream) const;
};

bool JumpInfoGenerator::Generate() const
{
	util::cppformatstream hstream;
	util::cppformatstream cstream;

	GenerateSource(cstream);
	GenerateHeader(hstream);

	std::ofstream hof (Manager.GetTarget() + "/jumpinfo.h");
	std::ofstream cof (Manager.GetTarget() + "/jumpinfo.cpp");

	hof << hstream.str();
	cof << cstream.str();

	return true;
}

bool JumpInfoGenerator::GenerateSource(util::cppformatstream &stream) const
{
	const gensim::arch::ArchDescription &arch = Manager.GetArch();

	stream << "#include \"jumpinfo.h\"\n";
	stream << "#include \"decode.h\"\n";
	stream << "using namespace gensim; using namespace gensim::" << Manager.GetArch().Name << ";";
	stream << "void JumpInfoProvider::GetJumpInfo(const gensim::BaseDecode *instr, archsim::Address pc, JumpInfo &info) {";

	stream << "info.IsJump = info.IsIndirect = info.IsConditional = false;";
	stream << "const Decode *inst = static_cast<const Decode*>(instr);";

	stream << "if(!instr->GetEndOfBlock()) { return; }";

	stream << "switch(inst->Instr_Code) {";

	for (std::list<isa::ISADescription *>::const_iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
		const isa::ISADescription *isa = *II;

		for (isa::ISADescription::InstructionDescriptionMap::const_iterator i = isa->Instructions.begin(); i != isa->Instructions.end(); ++i) {
			if (i->second->VariableJump || i->second->FixedJump || i->second->FixedJumpPred) {
				stream << "case INST_" << isa->ISAName << "_" << i->first << ":\n";
				stream << "info.IsJump = true;";
				if (i->second->VariableJump) stream << "info.IsIndirect = true;";
				if (i->second->FixedJumpPred) stream << "info.IsConditional = true;";
				if (i->second->FixedJump || i->second->FixedJumpPred) {
					assert(i->second->FixedJumpField.length() != 0);
					// Fixed jump
					if (i->second->FixedJumpType == 0) {
						stream << "info.JumpTarget = archsim::Address(inst->" << i->second->FixedJumpField << ");";
					}
					// Relative jump
					else {
						stream << "info.JumpTarget = pc + inst->" << i->second->FixedJumpField << " + " << i->second->FixedJumpOffset << ";";
					}
				}
				stream << "break;\n";
			}
		}
	}
	stream << "}\n";

	stream << "}\n";
	return true;
}

bool JumpInfoGenerator::GenerateHeader(util::cppformatstream &stream) const
{
	stream << "#include <cstdint>\n";
	stream << "#include <gensim/gensim_translate.h>\n";

	stream <<
	       "namespace gensim { "

	       // Forward decls
	       //
	       "class BaseDecode;\n"
	       "class Processor;\n "
	       "namespace " << Manager.GetArch().Name << "{ \n"
	       "class JumpInfoProvider : public gensim::BaseJumpInfoProvider {"
	       "public: void GetJumpInfo(const gensim::BaseDecode *instr, archsim::Address pc, JumpInfo &info) override;\n"
	       "};"
	       "}"
	       "}";

	return true;
}

DEFINE_COMPONENT(JumpInfoGenerator, jumpinfo)

