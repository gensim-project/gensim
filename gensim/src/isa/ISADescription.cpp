/*
 * File:   ISADescription.cpp
 * Author: s0803652
 *
 * Created on 27 September 2011, 14:20
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include <list>
#include <sstream>
#include <vector>

#include "isa/ISADescription.h"
#include "isa/HelperFnDescriptionParser.h"
#include "genC/ssa/SSAContext.h"
#include "Util.h"

#include "antlr-ver.h"
#include "genC/Parser.h"

using namespace gensim;
using namespace gensim::isa;

ISADescription::ISADescription(uint8_t isa_mode) : var_length_insns(false), insn_len(0), Formats(formats_), Instructions(instructions_), Mappings(mappings_), ExecuteActions(execute_actions_), BehaviourActions(behaviour_actions_), fetchLengthSet(false), valid(true), isa_mode_id(isa_mode), ssa_ctx_(nullptr)
{
}

ISADescription::~ISADescription()
{
	for (std::map<std::string, InstructionFormatDescription *>::iterator i = formats_.begin(); i != formats_.end(); i++) {
		delete i->second;
	}

	if(ssa_ctx_ != nullptr) {
		delete ssa_ctx_;
	}

	formats_.clear();
}

bool ISADescription::BuildSSAContext(gensim::arch::ArchDescription *arch, gensim::DiagnosticContext &diag_ctx)
{
	GASSERT(ssa_ctx_ == nullptr);
	if(ExecuteFiles.empty()) {
		return false;
	}

	bool success = true;
	genc::GenCContext *context = new genc::GenCContext(*arch, *this, diag_ctx);
	for (std::vector<std::string>::const_iterator ci = ExecuteFiles.begin(); ci != ExecuteFiles.end(); ci++) {
		if(!context->AddFile(*ci)) {
			success = false;
		}
	}

	if(success) success = context->Parse();

	if(!success) {
		return success;
	}

	if (context->IsValid()) {
		gensim::genc::ssa::SSAContext *isa_ssa_ctx;

		isa_ssa_ctx = context->EmitSSA();

		if(context->IsValid()) {
			if (!isa_ssa_ctx->Resolve(diag_ctx)) {
				return false;
			} else {
				ssa_ctx_ = isa_ssa_ctx;
				return true;
			}
		} else {
			return false;
		}
	} else {
		return false;
	}
}


bool ISADescription::HasOrthogonalFields(bool verbose) const
{
	typedef std::pair<uint8_t, uint8_t> field_span_t;
	bool success = true;
	std::map<std::string, field_span_t> field_spans;
	for (auto format : formats_) {
		uint8_t pos = 0;
		for (auto chunk : format.second->GetChunks()) {
			field_span_t f = {pos, pos + chunk.length};
			// If this is a field we care about, and it does not exactly match an identically named field we've already seen, bail out
			if (chunk.generate_field && !chunk.is_constrained && (field_spans.find(chunk.Name) != field_spans.end()) && field_spans.at(chunk.Name) != f) {
				success = false;
				if (verbose) {
					fprintf(stderr, "[ISA] Format %s: field %s is not orthogonal with identically named fields\n", format.first.c_str(), chunk.Name.c_str());
				}
			}
			field_spans[chunk.Name] = f;
			pos += chunk.length;
		}
	}
	return success;
}

bool ISADescription::IsFieldOrthogonal(const std::string &field) const
{
	// not currently working so disable orthogonalness
	return false;

	bool pair_valid = false;
	std::pair<uint8_t, uint8_t> p;
	for (auto format : formats_) {
		uint8_t pos = 0;
		for (auto chunk : format.second->GetChunks()) {
			if (chunk.Name == field) {
				std::pair<uint8_t, uint8_t> c_pair = std::make_pair(pos, pos + chunk.length);
				if (pair_valid && (c_pair != p)) return false;
				pair_valid = true;
				p = c_pair;
			}
		}
	}
	// We only consider the field orthogonal if it actually exists
	return pair_valid;
}

std::map<std::string, uint32_t> &ISADescription::Get_Decode_Fields() const
{
	if (fieldsSet) return fields;
	for (std::map<std::string, InstructionFormatDescription *>::const_iterator i = Formats.begin(); i != Formats.end(); ++i) {

		for (std::list<InstructionFormatChunk>::const_iterator chunk = i->second->GetChunks().begin(); chunk != i->second->GetChunks().end(); ++chunk) {
			if (chunk->is_constrained || !chunk->generate_field) continue;
			if (fields.find(chunk->Name) != fields.end()) {
				uint32_t currWidth = fields[chunk->Name];
				if (currWidth < chunk->length) fields[chunk->Name] = chunk->length;
			} else {
				fields[chunk->Name] = chunk->length;
			}
		}
	}
	fieldsSet = true; 
	return fields;
}

std::map<std::string, std::string> &ISADescription::Get_Disasm_Fields() const
{
	if (disasmFieldsSet) return disasmFields;
	for (std::map<std::string, InstructionFormatDescription *>::const_iterator i = Formats.begin(); i != Formats.end(); ++i) {

		for (std::list<InstructionFormatChunk>::const_iterator chunk = i->second->GetChunks().begin(); chunk != i->second->GetChunks().end(); ++chunk) {
			if (chunk->is_constrained || !chunk->generate_field) continue;
			std::string type = "uint8";
			if (chunk->length > 8) type = "uint16";
			if (chunk->length > 16) type = "uint32";
			disasmFields[chunk->Name] = type;
		}
	}
	for (std::list<FieldDescription>::const_iterator i = UserFields.begin(); i != UserFields.end(); ++i) {
		disasmFields[i->field_name] = i->field_type;
	}

	disasmFields["Instr_Length"] = "uint32";
#ifndef FLAG_PROPERTIES
	disasmFields["IsPredicated"] = "uint8";
#endif
#ifdef ENABLE_LIMM_OPERATIONS
	disasmFields["LimmBytes"] = "uint8";
	disasmFields["LimmPtr"] = "uint32*";
#endif

	disasmFieldsSet = true;
	return disasmFields;
}

std::map<std::string, std::string> ISADescription::Get_Hidden_Fields()
{
	std::map<std::string, std::string> fields;
	for (std::map<std::string, InstructionFormatDescription *>::const_iterator i = Formats.begin(); i != Formats.end(); ++i) {

		for (std::list<InstructionFormatChunk>::const_iterator chunk = i->second->GetChunks().begin(); chunk != i->second->GetChunks().end(); ++chunk) {
			if (chunk->generate_field || chunk->is_constrained) continue;
			std::string type = "uint8";
			if (chunk->length > 8) type = "uint16";
			if (chunk->length > 16) type = "uint32";
			fields[chunk->Name] = type;
		}
	}

	return fields;
}

std::list<std::string> ISADescription::GetPropertyList() const
{
	std::list<std::string> rVal;
	for (std::map<std::string, std::string>::const_iterator i = Properties.begin(); i != Properties.end(); ++i) {
		rVal.push_back(i->first);
	}
	return rVal;
}

void ISADescription::CleanupBehaviours()
{
	std::set<std::string> used_behaviours;

	for (InstructionDescriptionMap::iterator i = instructions_.begin(); i != instructions_.end(); ++i) {
		used_behaviours.insert(i->second->BehaviourName);
	}

	std::vector<std::string> remove_behaviours;
	for (ActionMap::iterator i = execute_actions_.begin(); i != execute_actions_.end(); ++i) {
		if (used_behaviours.find(i->first) == used_behaviours.end()) remove_behaviours.push_back(i->first);
	}

	for (std::vector<std::string>::iterator i = remove_behaviours.begin(); i != remove_behaviours.end(); ++i) {
		if (execute_actions_[*i] != NULL) delete execute_actions_[*i];
		execute_actions_.erase(*i);
	}
}

void ISADescription::AddHelperFn(const std::string& prototype, const std::string& body)
{
	std::stringstream prototype_stream(prototype);
	HelperFnDescriptionLexer lexer(prototype_stream);
	HelperFnDescriptionParser parser(lexer);

	auto descr = parser.Parse(body);
	if (descr == nullptr) {
		throw std::logic_error("Failed to parse helper function: '" + prototype + "'");
	}

	HelperFns.push_back(*descr);
	delete descr;
}
