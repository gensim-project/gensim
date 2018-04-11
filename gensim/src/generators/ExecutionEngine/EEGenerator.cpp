/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "generators/ExecutionEngine/EEGenerator.h"

using namespace gensim::generator;

bool EEGenerator::Generate() const
{
	bool success = true;
	util::cppformatstream header_str;
	util::cppformatstream source_str;

	// generate the source streams
	success &= GenerateHeader(header_str);
	success &= GenerateSource(source_str);

	if (success) {
		WriteOutputFile("ee_" + GetName() + ".h", header_str);
		WriteOutputFile("ee_" + GetName() + ".cpp", source_str);
	}
	return success;
}

std::string EEGenerator::GetFunction() const
{
	return "ExecutionEngine";
}

const std::vector<std::string> EEGenerator::GetSources() const
{
	return {"ee_" + GetName() + ".cpp"};
}

bool EEGenerator::GenerateHeader(util::cppformatstream &str) const {
	return true;
}

bool EEGenerator::GenerateSource(util::cppformatstream &str) const {
	return true;
}


EEGenerator::~EEGenerator()
{

}
