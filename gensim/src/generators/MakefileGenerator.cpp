/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <fstream>

#include "arch/ArchDescription.h"
#include "generators/MakefileGenerator.h"
#include "generators/GenerationManager.h"

DEFINE_COMPONENT(gensim::generator::MakefileGenerator, makefile)
COMPONENT_OPTION(makefile, llvm_path, "", "The path to the LLVM install to use for translation/JIT compilation")
COMPONENT_OPTION(makefile, archsim_path, "", "The path to the local Archsim install");
COMPONENT_OPTION(makefile, libtrace_path, "", "The path to the libtrace install");
COMPONENT_OPTION(makefile, Debug, "0", "Set to 1 to enable debug symbols during compilation");
COMPONENT_OPTION(makefile, Optimise, "1", "The optimisation level (-O[n]) to use during compilation");
COMPONENT_OPTION(makefile, Tune, "0", "Set to 1 to pass -march=native -mtune=native to the compiler");

namespace gensim
{
	namespace generator
	{

		typedef std::vector<std::string>::const_iterator string_iterator;

		MakefileGenerator::~MakefileGenerator() {}

		void MakefileGenerator::Reset()
		{
			objects.clear();
			sources.clear();
			preBuild.clear();
			postBuild.clear();
		}

		bool MakefileGenerator::Generate() const
		{

			std::ofstream makefile((Manager.GetTarget() + "/Makefile").c_str());

			makefile << "UNAME := $(shell uname)\n"
			         "LLVM_INCLUDE=" << GetProperty("llvm_path") << "/include\n"
			         "ARCHSIM_INCLUDE=" << GetProperty("archsim_path") << "\n"
			         "LIBTRACE_INCLUDE=" << GetProperty("libtrace_path") << "\n"
			         "CXX=g++\n"
			         "CFLAGS= -std=c++11 -fPIC -I$(ARCHSIM_INCLUDE) -I$(LLVM_INCLUDE) -I$(LIBTRACE_INCLUDE) -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -fno-rtti -fmax-errors=10";
			if (GetProperty("Debug") == "1") makefile << " -g ";
			makefile << " -O" << GetProperty("Optimise") << " ";
			if (GetProperty("Tune") == "1") makefile << " -mtune=native -march=native ";

			makefile << "\n"
			         "LDFLAGS= --shared -fPIC \n"
			         "ifeq ($(UNAME), Darwin)\n"
			         "\tLDFLAGS+=  -lLLVMCore -lLLVMSupport -L$(LLVM_INCLUDE)/../lib -lsim -L$(ARCHSIM_INCLUDE)/../lib\n"
			         "endif\n"
			         "SOURCES=";

			std::vector<std::string> final_sources = std::vector<std::string>(sources);
			const std::vector<GenerationComponent *> &components = Manager.GetComponents();
			for (std::vector<GenerationComponent *>::const_iterator ci = components.begin(); ci != components.end(); ++ci) {
				std::vector<std::string> ci_sources = (*ci)->GetSources();
				for (string_iterator si = ci_sources.begin(); si != ci_sources.end(); ++si) {
					final_sources.push_back(*si);
				}
			}

			for (string_iterator source = final_sources.begin(); source != final_sources.end(); ++source) {
				makefile << " " << *source;
			}

			makefile << "\n";

			makefile << "OBJECTS= $(SOURCES:.cpp=.o) ";
			for (string_iterator object = objects.begin(); object != objects.end(); ++object) {
				makefile << " " << *object;
			}

			makefile << "\n\n"
			         "default: post\n\n";

			makefile << "pre:\n";
			makefile << "ifeq ($(UNAME), Linux)\n";
			for (string_iterator pre_step = preBuild.begin(); pre_step != preBuild.end(); ++pre_step) {
				makefile << "\t" << *pre_step << "\n";
			}
			makefile << "else ifeq ($(UNAME), Darwin)\n"
			         "\tcat precompiled_insns.bc | ( echo \"unsigned char _binary_precompiled_insns_bc_start[] = {\"; xxd -i; echo \"};\" ) > output_file.c\n"
			         "\t(echo \"unsigned char* _binary_precompiled_insns_bc_end = _binary_precompiled_insns_bc_start + \";stat -f %z precompiled_insns.bc ;echo \";\")>> output_file.c\n"
			         "\tcc -c output_file.c -o precompiled_insns.o\n"
			         "endif\n";

			makefile << "\n\n"
			         "post: link\n";
			for (string_iterator post_step = postBuild.begin(); post_step != postBuild.end(); ++post_step) {
				makefile << "\t" << *post_step << "\n";
			}

			makefile << "\n\n"
			         "link: pre $(OBJECTS)\n";
			makefile << "\t$(CXX) $(LDFLAGS) $(OBJECTS) -o " << Manager.GetArch().Name << ".dll\n";

			makefile << ".cpp.o:\n";
			makefile << "\t$(CXX) $(CFLAGS) -c $< -o $@";

			makefile << "\n\n"
			         "clean: \n"
			         "\trm $(OBJECTS)\n"
			         "\trm " << Manager.GetArch().Name << ".dll\n";

			makefile.close();

			return true;
		}

		void MakefileGenerator::AddObjectFile(std::string objName)
		{
			objects.push_back(objName);
		}

		void MakefileGenerator::AddPreBuildStep(std::string step)
		{
			preBuild.push_back(step);
		}

		const std::vector<std::string> MakefileGenerator::GetSources() const
		{
			std::vector<std::string> sources;
			return sources;
		}

	}  // namespace generator
}  // namespace gensim
