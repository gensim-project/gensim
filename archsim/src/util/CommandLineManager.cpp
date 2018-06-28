/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <cstring>
#include <sstream>

#define __IMPLEMENT_OPTIONS
#include "util/SimOptions.h"
#undef  __IMPLEMENT_OPTIONS

#include "util/CommandLine.h"

using namespace archsim::util;

#define DefineFlag(_o, _s, _l) static SimOptionArgumentDescriptor<bool> __attribute__((init_priority(130))) _o(_s, _l, archsim::options::_o)
#define DefineOptionalArgument(_t, _o, _s, _l) static SimOptionArgumentDescriptor<_t> __attribute__((init_priority(130))) _o(_s, _l, archsim::options::_o, ArgumentDescriptor::ValueOptional)
#define DefineRequiredArgument(_t, _o, _s, _l) static SimOptionArgumentDescriptor<_t> __attribute__((init_priority(130))) _o(_s, _l, archsim::options::_o, ArgumentDescriptor::ValueRequired)

#define DefineLongFlag(_o, _l) static SimOptionArgumentDescriptor<bool> __attribute__((init_priority(130))) _o(_l, archsim::options::_o)
#define DefineLongOptionalArgument(_t, _o, _l) static SimOptionArgumentDescriptor<_t> __attribute__((init_priority(130))) _o(_l, archsim::options::_o, ArgumentDescriptor::ValueOptional)
#define DefineLongRequiredArgument(_t, _o, _l) static SimOptionArgumentDescriptor<_t> __attribute__((init_priority(130))) _o(_l, archsim::options::_o, ArgumentDescriptor::ValueRequired)

#include "util/CommandLineOptions.h"

std::map<char, ArgumentDescriptor *> __attribute__((init_priority(110))) CommandLineManager::ShortMap;
std::map<std::string, ArgumentDescriptor *> __attribute__((init_priority(110))) CommandLineManager::LongMap;
std::list<ArgumentDescriptor *> __attribute__((init_priority(110))) CommandLineManager::RegisteredArguments;

std::map<std::string, std::list<archsim::SimOptionBase *> > __attribute__((init_priority(110))) archsim::SimOptionBase::RegisteredOptions;

int CommandLineManager::guest_argc;
char **CommandLineManager::guest_argv;

CommandLineManager::CommandLineManager()
{

}

CommandLineManager::~CommandLineManager()
{

}

namespace archsim
{
	namespace util
	{

		template<typename T>
		void SimOptionArgumentDescriptor<T>::Marshal(std::string str)
		{
			option.SetValue(str);
		}

		template <>
		void SimOptionArgumentDescriptor<uint32_t>::Marshal(std::string str)
		{
			option.SetValue(atoi(str.c_str()));
		}

		template <>
		void SimOptionArgumentDescriptor<bool>::Marshal(std::string str)
		{
			option.SetValue(str == "true");
		}

		template <>
		void SimOptionArgumentDescriptor<std::list<std::string> *>::Marshal(std::string str)
		{
			std::list<std::string>& device_list = *option.GetValue();
			std::stringstream requested_devices(str);
			std::string device;

			while (std::getline(requested_devices, device, ',')) {
				device_list.push_back(device);
			}
		}
	}
}

void CommandLineManager::PrintUsage()
{
	fprintf(stderr, "Usage:\n");

	for (auto cla : RegisteredArguments) {
		fprintf(stderr, "  ");

		if (cla->HasShort()) {
			fprintf(stderr, "-%c  ", cla->GetShort());
		} else {
			fprintf(stderr, "    ");
		}

		if (cla->HasLong()) {
			fprintf(stderr, "--%-20s ", cla->GetLong().c_str());
		} else {
			fprintf(stderr, "                      ");
		}

		if (cla->value_validity == ArgumentDescriptor::ValueForbidden) {
			fprintf(stderr, "    ");
		} else if (cla->value_validity == ArgumentDescriptor::ValueOptional) {
			fprintf(stderr, "[?] ");
		} else if (cla->value_validity == ArgumentDescriptor::ValueRequired) {
			fprintf(stderr, "<?> ");
		}

		fprintf(stderr, "%s\n", cla->GetDescription().c_str());
	}
}

bool CommandLineManager::Parse(int argc, char **argv)
{
	int guest_program_args_start = -1;
	ArgumentDescriptor *descriptor;

	guest_argc = 0;
	guest_argv = NULL;

	binary_name = std::string(argv[0]);

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == '-') {
				if (argv[i][2] == '\0') {
					// We've just found '--', so we need to stop parsing, and remember
					// the starting location of the guest program arguments.
					guest_program_args_start = i + 1;
					break;
				} else {
					// We've just found a long command-line argument.
					if (!FindDescriptor(std::string(&argv[i][2]), descriptor)) {
						fprintf(stderr, "error: invalid command-line argument %s\n", &argv[i][2]);
						return false;
					}

					descriptor->MarkPresent();

					switch (descriptor->value_validity) {
						case ArgumentDescriptor::ValueForbidden:
							descriptor->Marshal("true");
							break;

						case ArgumentDescriptor::ValueOptional:
							if ((i + 1) < argc && argv[i + 1][0] != '-') {
								descriptor->Marshal(std::string(argv[i+1]));
								i++;
							}
							break;

						case ArgumentDescriptor::ValueRequired:
							if (i < (argc-1) && argv[i+1][0] != '-') {
								descriptor->Marshal(std::string(argv[i+1]));
								i++;
							} else {
								fprintf(stderr, "error: command-line argument %s requires a value\n", &argv[i][2]);
								return false;
							}
							break;
					}
				}
			} else {
				int sequence_length = strlen(argv[i]);

				// We've just found a short (or sequence of short) command-line arguments.
				for (int n = 1; n < sequence_length; n++) {
					if (!FindDescriptor(argv[i][n], descriptor)) {
						fprintf(stderr, "error: invalid command-line argument %c\n", argv[i][n]);
						return false;
					}

					descriptor->MarkPresent();

					// If we're on the last flag in the sequence
					if (n == sequence_length - 1) {
						switch (descriptor->value_validity) {
							case ArgumentDescriptor::ValueForbidden:
								descriptor->Marshal("true");
								break;

							case ArgumentDescriptor::ValueOptional:
								if ((i + 1) < argc && argv[i + 1][0] != '-') {
									descriptor->Marshal(std::string(argv[i+1]));
									i++;
								}
								break;

							case ArgumentDescriptor::ValueRequired:
								if (i < (argc-1) && argv[i+1][0] != '-') {
									descriptor->Marshal(std::string(argv[i+1]));
									i++;
								} else {
									fprintf(stderr, "error: command-line argument %c requires a value\n", argv[i][n]);
									return false;
								}
								break;
						}
					} else if (descriptor->value_validity == ArgumentDescriptor::ValueRequired) {
						// If we're not on the last flag in the sequence, and
						// we expect a value, then this is an error.
						fprintf(stderr, "error: command-line argument %c requires a value\n", argv[i][n]);
						return false;
					} else {
						descriptor->Marshal("true");
					}
				}
			}
		} else {
			fprintf(stderr, "error: unexpected command-line argument value %s\n", argv[i]);
			return false;
		}
	}

	if (guest_program_args_start >= 0) {
		guest_argc = argc - guest_program_args_start;
		guest_argv = &argv[guest_program_args_start];
	}

	return true;
}

bool CommandLineManager::FindDescriptor(std::string long_argument, ArgumentDescriptor*& handler)
{
	auto descr = LongMap.find(long_argument);
	if (descr == LongMap.end())
		return false;

	handler = descr->second;
	return true;
}

bool CommandLineManager::FindDescriptor(char short_argument, ArgumentDescriptor*& handler)
{
	auto descr = ShortMap.find(short_argument);
	if (descr == ShortMap.end())
		return false;

	handler = descr->second;
	return true;
}

// Not really a great place for this without inventing a new file.  And we HATE new files.
namespace archsim
{
	namespace options
	{
		void PrintOptions()
		{
			fprintf(stderr, "Simulation Options:\n");
			for (auto category : SimOptionBase::RegisteredOptions) {
				fprintf(stderr, "%s:\n", category.first.c_str());
				for (auto option : category.second) {
					fprintf(stderr, "  %-20s: %-50s\n", option->GetName().c_str(), option->GetDescription().c_str());
				}
				fprintf(stderr, "\n");
			}
		}
	}
}
