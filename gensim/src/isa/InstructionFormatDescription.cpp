/*
 * InstructionFormatDescription.cpp
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#include <cstring>

#include <iostream>
#include <set>
#include <sstream>

#include "isa/InstructionFormatDescription.h"
#include "define.h"

using namespace gensim;
using namespace gensim::isa;

InstructionFormatChunk ParseChunk(char *tok, uint8_t &length)
{
	// return a new instruction format chunk:
	//  name has the initial % trimmed off
	//  the length starts just after the first colon - we rely on atoi to discard the signedness
	//    indicator, if it exists.
	if (tok[0] == '%') {
		// get the position of the colon separating the name and length
		char *colon1 = strchr(tok, ':');
		assert(*colon1);

		// copy the name out of the chunk string
		std::string name (tok, 0, colon1-tok);

		bool generate_field = true;
		if (name[colon1 - tok - 1] == '!') {
			generate_field = false;
			name[colon1 - tok - 1] = '\0';
		}

		//    sscanf(tok, "\%%s:%u", name, &length);

		length = atoi(colon1 + 1);
		return InstructionFormatChunk(name.substr(1).c_str(), length, false, generate_field);
	} else if (tok[0] == '0') {
		// this is a format-specified constrainment
		InstructionFormatChunk chunk = InstructionFormatChunk("Static Constrainment", 0, false, false);
		if (sscanf(tok, "0x%x:%hhu", &chunk.constrained_value, &chunk.length) != 2) {
			fprintf(stderr, "Invalid format string\n");
			exit(1);
		}
		length = chunk.length;
		chunk.is_constrained = true;
		return chunk;
	}
	fprintf(stderr, "Invalid format chunk syntax '%s'\n", tok);
	exit(1);
}

bool InstructionFormatDescription::hasChunk(std::string name) const
{
	for (std::list<InstructionFormatChunk>::const_iterator i = Chunks.begin(); i != Chunks.end(); ++i) {
		if (i->Name == name) return true;
	}
	return false;
}

const InstructionFormatChunk &InstructionFormatDescription::GetChunkByName(std::string name) const
{
	for (const auto &chunk : Chunks)
		if (chunk.Name == name) return chunk;
	GASSERT(!"Could not find chunk");
	UNREACHABLE;
}

std::string InstructionFormatDescription::ToString()
{
	// output the name of this format, then look over each field and print its name and length
	std::stringstream buf;
	buf << Name << ": ";
	for (std::list<InstructionFormatChunk>::iterator i = Chunks.begin(); i != Chunks.end(); i++) {
		if (i->is_constrained)
			buf << "0x" << std::hex << i->constrained_value << ":" << std::dec << (uint32_t)i->length << " ";
		else
			buf << "%" << i->Name << (i->generate_field ? "" : "!") << ":" << (uint32_t)(i->length) << " ";
	}
	return buf.str();
}

InstructionFormatDescriptionParser::InstructionFormatDescriptionParser(DiagnosticContext &diag, const ISADescription& isa) : diag(diag), isa_(isa)
{

}

bool InstructionFormatDescriptionParser::Parse(const std::string &name, const std::string &format, InstructionFormatDescription *&description)
{
	description = new InstructionFormatDescription(isa_);

	// create the new instruction format and set its name
	description->SetName(name);

	// parse each field and add it to the format
	char *format_tok = strdup(format.c_str());

	char *tok = strtok(format_tok, " ");
	uint8_t chunkLength;
	while (tok != NULL) {
		description->AddChunk(ParseChunk(tok, chunkLength));
		tok = strtok(NULL, " ");
	}

	free(format_tok);

	// check that format is valid
	bool success = true;
	std::set<std::string> FormatNames;
	for (std::list<InstructionFormatChunk>::const_iterator ci = description->GetChunks().begin(); ci != description->GetChunks().end(); ++ci) {
		if (ci->Name == "Static Constrainment") continue;
		if (FormatNames.count(ci->Name)) {
			diag.Error("Format " + name + " uses field name " + ci->Name + " multiple times", DiagNode("", 0, 0));
			success = false;
			continue;
		}
		FormatNames.insert((ci)->Name);
	}

	return success;
}
