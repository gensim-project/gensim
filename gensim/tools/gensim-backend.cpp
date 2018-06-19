/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <fstream>

#include "arch/ArchDescription.h"
#include "arch/ArchDescriptionParser.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "DiagnosticContext.h"
#include "genC/InstStructBuilder.h"

#include "Util.h"

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include "config_gensim.h"

#include "generators/GenerationManager.h"

#include "genC/Parser.h"
#include "UArchDescription.h"

using namespace gensim;
using namespace gensim::generator;
using namespace gensim::util;

void EnsureDirectoryExists(const char *path)
{
	struct stat m;
	if (stat(path, &m) != 0) {
		if (errno == ENOENT) {
			// Directory doesn't exist: create it
			// NB there is a race condition here.
			if (mkdir(path, 0755) == 0) {
				return;
			} else {
				fprintf(stderr, "Could not create %s/: %s", path, strerror(errno));
				abort();
			}
		} else {
			fprintf(stderr, "Could not stat %s/: %s", path, strerror(errno));
			abort();
		}
	}
	// $path/ exists - is it a directory?
	if (!S_ISDIR(m.st_mode)) {
		fprintf(stderr, "%s/ exists, but is not a directory", path);
		abort();
	}
}
static bool verbose_flag = false;

static struct option long_options[] = {
	{"arch", required_argument, 0, 'a'},
	{"help", no_argument, 0, 'h'},
	{"stage_opt", required_argument, 0, 'o'},
	{"verbose", optional_argument, 0, 'v'},
	{"add_stage", required_argument, 0, 's'},
	{"target", required_argument, 0, 't'},
	{0, 0, 0, 0}
};

void print_usage()
{
	// get terminal size so that we can print nicely
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	std::cout << "Gensim usage\n"
	          "  gensim [options] -a [archname].ac -t [target folder]\n"
	          "\n"
	          "Options:\n"
	          "  --arch, -a:      Specify the architecture file to generate from.\n"
	          "  --help, -h:      Show this usage infomation\n"
	          "  --stage_opt, -o: Specify an option for a generation stage in the format \n"
	          "                   [stage].[option]=[value]\n"
	          "  --verbose, -v:   Run the generation in a more verbose mode\n"
	          "  --add_stage, -s: Add a generation stage.\n"
	          "  --target, -t:    Specify the output folder\n\n"
	          ""
	          "Valid stages:\n";

	for (std::list<GenerationComponentInitializer *>::const_iterator ci = GenerationComponentInitializer::Initializers.begin(); ci != GenerationComponentInitializer::Initializers.end(); ++ci) {
		if ((*ci)->dummy) continue;
		std::string comp_name = (*ci)->GetName();
		std::cout << "\t" << comp_name << "\n";

		// now print any properties of the component
		if (GenerationComponent::Options.find(comp_name) != GenerationComponent::Options.end())
			for (std::map<std::string, GenerationOption *>::const_iterator opt_i = GenerationComponent::Options[comp_name].begin(); opt_i != GenerationComponent::Options[comp_name].end(); ++opt_i) {
				GenerationOption *init = opt_i->second;

				std::cout << "\t\t" << init->Name << " [" << init->DefaultValue << "] " << init->HelpString << std::endl;
			}

		// now print any properties which the component inherits
		if (GenerationComponent::Inheritance.find(comp_name) != GenerationComponent::Inheritance.end()) {
			do {
				comp_name = GenerationComponent::Inheritance[comp_name];
				if (GenerationComponent::Options.find(comp_name) != GenerationComponent::Options.end())
					for (std::map<std::string, GenerationOption *>::const_iterator opt_i = GenerationComponent::Options[comp_name].begin(); opt_i != GenerationComponent::Options[comp_name].end(); ++opt_i) {
						GenerationOption *init = opt_i->second;
						std::cout << "\t\t" << init->Name << " [" << init->DefaultValue << "] " << init->HelpString << std::endl;
					}
			} while (GenerationComponent::Inheritance.find(comp_name) != GenerationComponent::Inheritance.end());
		}
	}
}

static void DumpDiagnostics(const DiagnosticContext& context)
{
	std::cout << context;
}

/*
 *
 */
//#define TEST_PARSER

int main(int argc, char **argv)
{

	if (argc <= 1) {
		print_usage();
		return 0;
	}

	std::vector<std::string> component_types;
	std::map<std::string, std::map<std::string, std::string> > component_options;

	std::string output_folder = "output/";
	std::string arch_name;

	bool success = true;

	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "a:ho:s:t:v:", long_options, &option_index);
		if (c == -1) break;

		switch (c) {
			case 'a':
				arch_name = optarg;
				break;
			case 'h':
				print_usage();
				return 0;
			case 'v':
				if (optarg) {
					Util::Verbose_Level = atoi(optarg);
				} else
					Util::Verbose_Level = true;
				break;
			case 's': {
				// Figure out which component the user is trying to add and add it to the generator
				std::vector<std::string> new_components = Util::Tokenize(optarg, ",", false);

				for (std::vector<std::string>::iterator i = new_components.begin(); i != new_components.end(); ++i) {
					component_types.push_back(*i);
					component_options[*i] = std::map<std::string, std::string>();
				}

				break;
			}
			case 'o': {
				// Parse the option (it appears in the form component.option=value)

				std::string option_string = optarg;
				std::vector<std::string> new_options = Util::Tokenize(optarg, ",", false);

				for (std::vector<std::string>::iterator i = new_options.begin(); i != new_options.end(); ++i) {
					std::string &option_string = *i;
					uint8_t dot_loc = option_string.find('.', 0);
					uint8_t eq_loc = option_string.find('=', 0);

					std::string component = option_string.substr(0, dot_loc);
					std::string option = option_string.substr(dot_loc + 1, eq_loc - dot_loc - 1);
					std::string value = option_string.substr(eq_loc + 1, option_string.npos);

					if (component_options.find(component) == component_options.end()) {
						std::cout << "[GENSIM] ERROR! Unrecognized component: " << component << std::endl;
						success = false;
						continue;
					}

					component_options[component][option] = value;
				}

				break;
			}
			case 't': {
				output_folder = optarg;
				break;
			}
		}
	}

	if (arch_name.empty()) {
		printf("Please specify an architecture file with the -a option.\n");
		exit(1);
	}

	DiagnosticSource root_source("GenSim");
	DiagnosticContext root_context(root_source);

	arch::ArchDescriptionParser parser(root_context);
	if (!parser.ParseFile(arch_name)) {
		parser.Get()->PrettyPrint(std::cout);
		DumpDiagnostics(root_context);

		return 1;
	}

	arch::ArchDescription &arch = *parser.Get();

	success = true;

	gensim::genc::ssa::io::AssemblyReader reader;
	gensim::genc::InstStructBuilder isb;
	for (std::list<isa::ISADescription *>::iterator II = arch.ISAs.begin(), IE = arch.ISAs.end(); II != IE; ++II) {
		isa::ISADescription *isa = *II;
		// look for a file called (isa name).gcs
		gensim::genc::ssa::SSAContext *isa_context = new gensim::genc::ssa::SSAContext(*isa, arch);
		isa_context->GetTypeManager().InsertStructType("Instruction", isb.BuildType(isa));

		gensim::genc::ssa::io::AssemblyFileContext *asm_ctx;
		std::string isa_filename = isa->ISAName + ".gcs";
		bool success = reader.Parse(isa_filename, root_context, asm_ctx);
		if(!success) {
			fprintf(stderr, "Could not parse assembly for isa %s\n", isa->ISAName.c_str());
			return 1;
		}

		gensim::genc::ssa::io::ContextAssembler ca;

		ca.SetTarget(isa_context);
		ca.Assemble(*asm_ctx);
		isa->SetSSAContext(isa_context);
		if(!isa_context->Resolve(root_context)) {
			fprintf(stderr, "Failed to resolve ISA %s!\n", isa->ISAName.c_str());
			return 1;
		}
	}

	if(!success) {
		DumpDiagnostics(root_context);
		return 1;
	}

#ifdef TEST_PARSER
	std::ostringstream errors;
	genc::GenCContext *root_context = genc::GenCContext::Parse("test_file", errors, arch);
	success = root_context->Resolve();
	std::cerr << errors.str();
	errors.str("");
	if (root_context) {
		root_context->PrettyPrint(errors);
	}
	std::cerr << errors.str();

	if (root_context->IsValid() && success) {
		std::ostringstream out;
		genc::ssa::SSAContext *ssa = root_context->EmitSSA();
		ssa->Resolve();
		ssa->PrettyPrint(out);
		std::cout << out.str();
	}
	return 0;
#endif

	EnsureDirectoryExists(output_folder.c_str());
	generator::GenerationManager gen(arch, output_folder);

	// generate_decoders(description);
	std::map<std::string, GenerationComponent *> components;

	for (std::vector<std::string>::iterator i = component_types.begin(); i != component_types.end(); ++i) {
		GenerationComponent *newcomponent = NULL;

		for (std::list<GenerationComponentInitializer *>::const_iterator ci = GenerationComponentInitializer::Initializers.begin(); ci != GenerationComponentInitializer::Initializers.end(); ++ci) {
			GenerationComponentInitializer &init = **ci;
			if (init.GetName() == *i) {
				newcomponent = init.Get(gen);
				break;
			}
		}

		if (!newcomponent) {
			root_context.Error("Unrecognised component: " + *i);
			success = false;
			continue;
		}

		root_context.Info("Creating component: " + *i);
		components[*i] = newcomponent;
		gen.AddComponent(*newcomponent);
	}

	for (std::map<std::string, std::map<std::string, std::string> >::iterator component = component_options.begin(); component != component_options.end(); ++component) {
		for (std::map<std::string, std::string>::iterator option = component->second.begin(); option != component->second.end(); ++option) {
			GenerationComponent *comp = components[component->first];
			if (comp->HasProperty(option->first)) {
				comp->SetProperty(option->first, option->second);
			} else {
				root_context.Error("Unrecognised property '" + option->first + "' of component '" + component->first + "'");
				success = false;
			}
		}
	}

	if (success) success &= gen.Generate();


	DumpDiagnostics(root_context);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

