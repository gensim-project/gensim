/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "generators/GenerationManager.h"

#include <vector>

using namespace gensim::generator;

class FunctionGenerator : public GenerationComponent { 
public:
	FunctionGenerator(GenerationManager &man) : GenerationComponent(man, "function") { }
	
	unsigned GetFileCount() const { return atoi(GetProperty("FileCount").c_str()); }
	
	virtual bool Generate() const {
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
			
			file_sizes[smallest_file] += fn_entry.GetBodySize();
			file_descriptors[smallest_file].push_back(fn_entry);
		}
		
		for(unsigned file_idx = 0; file_idx < GetFileCount(); ++file_idx) {
			std::stringstream str;
			
			str << "#include \"function_header.h\"\n";
			
			for(auto &fn_entry : file_descriptors[file_idx]) {
				str << fn_entry.Format() << "\n";
			}
			
			WriteOutputFile("src_" + std::to_string(file_idx) + ".cpp", str);
		}
		
		// also need to produce a header file containing declarations
		std::stringstream str;
		for(auto &fn_entry : Manager.GetFunctionEntries()) {
			// include dependent headers
			if(fn_entry.IsGlobal()){
				str << fn_entry.FormatIncludes();

				str << fn_entry.FormatPrototype() << ";\n";
			}
		}
		WriteOutputFile("function_header.h", str);
		
		return true;
	}

	virtual const std::vector<std::string> GetSources() const {
		std::vector<std::string> sources;
		for(unsigned i = 0; i < GetFileCount(); ++i) {
			sources.push_back("src_" + std::to_string(i) + ".cpp");
		}
		
		return sources;
	}

	virtual std::string GetFunction() const { return "Functions"; }

	virtual void Reset() {} 

	virtual void Setup(GenerationSetupManager& Setup) {}




};

DEFINE_COMPONENT(FunctionGenerator, function);
COMPONENT_OPTION(function, FileCount, "5", "The number of separate files to generate functions into.")