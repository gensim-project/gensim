/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/ISADescription.h"
#include "genC/InstStructBuilder.h"
#include "genC/Parser.h"

using namespace gensim::genc;

IRType StructBuilder::BuildStruct(const gensim::isa::ISADescription* isa, const gensim::isa::StructDescription* struct_desc, ssa::SSATypeManager &man) const
{
	IRStructType *stype = new IRStructType();
	stype->Name = struct_desc->GetName();

	for(auto member : struct_desc->GetMembers()) {
		stype->AddMember(member.GetName(), GetGenCType(isa, member.GetType(), man));
	}
	stype->MakeFullyConst();

	return IRType::CreateStruct(stype);
}


IRType StructBuilder::GetGenCType(const gensim::isa::ISADescription *isa, const std::string &typeName, ssa::SSATypeManager &man) const
{
	IRType type = IRTypes::Void;

	std::string raw_typename = gensim::util::Util::FindReplace(typeName, "*", "");

	if (!raw_typename.compare("uint8")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT8;
		type.Signed = false;
	} else if (!raw_typename.compare("uint16")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT16;
		type.Signed = false;
	} else if (!raw_typename.compare("uint32")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT32;
		type.Signed = false;
	} else if (!raw_typename.compare("uint64")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT64;
		type.Signed = false;
	} else if (!raw_typename.compare("sint32")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT32;
		type.Signed = true;
	} else if (!raw_typename.compare("sint64")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT64;
		type.Signed = true;
	} else if(isa->HasUserStructType(typeName)) {
		if(!man.HasStructType(typeName)) {
			man.InsertStructType(typeName, BuildStruct(isa, &isa->GetUserStructType(typeName), man));
		}
		type = man.GetStructType(typeName);
	} else {
		throw std::logic_error("Unknown type: " + typeName);
	}
	return type;
}

IRType InstStructBuilder::BuildType(const gensim::isa::ISADescription* isa, ssa::SSATypeManager &man) const
{
	gensim::isa::StructDescription inst_description("Instruction");

	inst_description.AddMember("Instr_Length", "uint8");
	inst_description.AddMember("Instr_Code", "uint32");
	inst_description.AddMember("ir", "uint32");
	inst_description.AddMember("End_Of_Block", "uint8");
	inst_description.AddMember("Uses_PC", "uint8");
	inst_description.AddMember("IsPredicated", "uint8");
	inst_description.AddMember("PredicateInfo", "uint32");
	inst_description.AddMember("IsBranch", "uint8");

	const std::map<std::string, std::string> &fields = isa->Get_Disasm_Fields();
	for (std::map<std::string, std::string>::const_iterator ci = fields.begin(); ci != fields.end(); ++ci) {
		inst_description.AddMember(ci->first, ci->second);
	}

	return StructBuilder().BuildStruct(isa, &inst_description, man);
}
