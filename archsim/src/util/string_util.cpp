/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "util/string_util.h"

#include <sstream>

std::vector<std::string> archsim::util::split_string(const std::string &input, char delim)
{
	std::vector<std::string> atoms;
	std::istringstream str (input);

	std::string string;
	while(std::getline(str, string, delim)) {
		atoms.push_back(string);
	}

	return atoms;
}

std::string archsim::util::join_string(const std::vector<std::string> &atoms, char delim)
{
	return join_string(atoms.begin(), atoms.end(), delim);
}
