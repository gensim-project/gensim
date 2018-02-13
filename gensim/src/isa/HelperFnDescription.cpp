/*
 * HelperFnDescription.cpp
 *
 *  Created on: 14 Jul 2015
 *      Author: harry
 */

#include "isa/HelperFnDescription.h"

#include <cassert>
#include <sstream>

using namespace gensim::isa;

#define CAPTIVE

std::string HelperFnDescription::GetPrototype(const std::string& class_prefix) const
{
	std::stringstream out;
	out << return_type;

#ifdef CAPTIVE
	if (return_type != "void") out << "_t";
#endif

	out << " ";

	if (class_prefix != "") {
		out << class_prefix << "::";
	}

	out << name << "(";

	bool first = true;
	for(const auto &param : params) {
		if(!first) out << ",";
		first = false;
		out << param.type;

#ifdef CAPTIVE
		out << "_t";
#endif

		out << " " << param.name;
	}

	out << ")";
	return out.str();
}

/*void HelperFnDescription::ParsePrototype(const std::string& incoming)
{
	std::string prototype = incoming;
	std::vector<std::string> tokens;
	while(prototype.find(' ') != prototype.npos) {
		std::string token = prototype.substr(0, prototype.find(' '));

		if(token == "inline") {
			attributes.push_back("inline");
			should_inline = true;
		} else if (token=="global") {
			attributes.push_back("global");
		} else if (token=="noinline") {
			attributes.push_back("noinline");
		} else {
			tokens.push_back(token);
		}
		prototype = prototype.substr(prototype.find(' ')+1, prototype.npos);
	}
	if(prototype != "")tokens.push_back(prototype);

	//get return type
	return_type = tokens[0];

	//get function name
	name = tokens[1];

	// tokens[2] = (

	if(tokens[2] == "(" && tokens[3] == ")") return;
	auto iterator = tokens.begin() + 3;
	while(iterator != tokens.end()) {
		HelperFnParamDescription param;
		//base type
		param.type = *iterator++;

		printf("XXXX %s\n", (*iterator).c_str());

		if (*iterator == "[") {
			param.type += "[";
			iterator++;
			param.type += *iterator;
			iterator++;
			param.type += "]";
			iterator++;
		} else if (*iterator == "&") {
			param.type += "&";
			iterator++;
		}

		param.name = *iterator++;

		params.push_back(param);
		assert(*iterator == "," || *iterator == ")");
		iterator++;
	}

}*/
