/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/ArchDescription.h"
#include "generators/GenerationManager.h"
#include "generators/ExternalJumpInfoGenerator.h"

using namespace gensim::generator;

bool ExternalJumpInfoGenerator::Generate() const
{
	std::stringstream str;
	str << "#include <arch/" << Manager.GetArch().Name << "/" << GetProperty("Filename") << ">\n";
	str << "namespace gensim { namespace " << Manager.GetArch().Name << " { using JumpInfoProvider = " << GetProperty("Class") << "; } }";
	WriteOutputFile("jumpinfo.h", str);
	return true;
}

using gensim::generator::ExternalJumpInfoGenerator;
DEFINE_COMPONENT(ExternalJumpInfoGenerator, external_jumpinfo);
COMPONENT_OPTION(external_jumpinfo, Filename, "jumpinfo.h", "The filename to use within archsim (archsim/inc/arch/{arch}/{Filename})");
COMPONENT_OPTION(external_jumpinfo, Class, "JumpInfoProvider", "The class to use within archsim for jump info");