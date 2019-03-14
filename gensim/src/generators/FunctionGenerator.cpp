/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/GenerationManager.h"

#include <vector>
#include <set>

using namespace gensim::generator;

class FunctionGenerator : public GenerationComponent
{
public:
	FunctionGenerator(GenerationManager &man) : GenerationComponent(man, "function") { }

	unsigned GetFileCount() const
	{
		return atoi(GetProperty("FileCount").c_str());
	}

	virtual bool Generate() const
	{
		// Distribute the registered functions among each file, trying to
		// keep the file sizes more or less equal. This is fairly naive in
		// this case since we just add each function to the currently smallest
		// file.

		std::vector<std::vector<FunctionEntry>> file_descriptors (GetFileCount());
		std::vector<size_t> file_sizes (GetFileCount());

		for(auto &fn_entry : Manager.GetFunctionEntries()) {

			int smallest_file = 0;
			for(unsigned i = 0; i < file_sizes.size(); ++i) {
				if(file_sizes[i] < file_sizes[smallest_file]) {
					smallest_file = i;
				}
			}

			file_sizes[smallest_file] += fn_entry.second.GetBodySize();
			file_descriptors[smallest_file].push_back(fn_entry.second);
		}

		for(unsigned file_idx = 0; file_idx < GetFileCount(); ++file_idx) {
			std::stringstream str;

			str << "#include \"function_header.h\"\n";

			std::set<std::string> local_headers;
			std::set<std::string> system_headers;

			for(auto &fn_entry : file_descriptors[file_idx]) {
				for(auto local_header : fn_entry.GetLocalHeaders()) {
					local_headers.insert(local_header);
				}
				for(auto system_header : fn_entry.GetSystemHeaders()) {
					system_headers.insert(system_header);
				}
			}

			for(auto i : local_headers) {
				str << "#include \"" << i << "\"\n";
			}
			for(auto i : system_headers) {
				str << "#include <" << i << ">\n";
			}

			for(auto &fn_entry : file_descriptors[file_idx]) {
				str << fn_entry.Format() << "\n";
			}

			WriteOutputFile("src_" + std::to_string(file_idx) + ".cpp", str);
		}

		// also need to produce a header file containing declarations
		std::stringstream str;
		std::set<std::string> local_headers;
		std::set<std::string> system_headers;

		for(auto &fn_entry : Manager.GetFunctionEntries()) {
			for(auto local_header : fn_entry.second.GetLocalHeaders()) {
				local_headers.insert(local_header);
			}
			for(auto system_header : fn_entry.second.GetSystemHeaders()) {
				system_headers.insert(system_header);
			}
		}
		// include dependent headers
		for(auto i : local_headers) {
			str << "#include \"" << i << "\"\n";
		}
		for(auto i : system_headers) {
			str << "#include <" << i << ">\n";
		}
		for(auto &fn_entry : Manager.GetFunctionEntries()) {
			if(fn_entry.second.IsGlobal()) {
				str << fn_entry.second.FormatPrototype() << ";\n";
			}
		}
		WriteOutputFile("function_header.h", str);

		return true;
	}

	virtual const std::vector<std::string> GetSources() const
	{
		std::vector<std::string> sources;
		for(unsigned i = 0; i < GetFileCount(); ++i) {
			sources.push_back("src_" + std::to_string(i) + ".cpp");
		}

		return sources;
	}

	virtual std::string GetFunction() const
	{
		return "Functions";
	}

	virtual void Reset() {}

	virtual void Setup(GenerationSetupManager& Setup) {}

};

DEFINE_COMPONENT(FunctionGenerator, function);
COMPONENT_OPTION(function, FileCount, "10", "The number of separate files to generate functions into.")
