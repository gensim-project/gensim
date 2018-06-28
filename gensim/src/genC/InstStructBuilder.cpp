/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/ISADescription.h"
#include "genC/InstStructBuilder.h"

using namespace gensim::genc;

IRType GetGenCType(std::string typeName)
{
	IRType type = IRTypes::Void;

	std::string::reverse_iterator c = typeName.rbegin();
	while (*c == '*') {
		c++;
	}
	typeName = gensim::util::Util::FindReplace(typeName, "*", "");

	if (!typeName.compare("uint8")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT8;
		type.Signed = false;
	} else if (!typeName.compare("uint16")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT16;
		type.Signed = false;
	} else if (!typeName.compare("uint32")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT32;
		type.Signed = false;
	} else if (!typeName.compare("uint64")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT64;
		type.Signed = false;
	} else if (!typeName.compare("sint32")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT32;
		type.Signed = true;
	} else if (!typeName.compare("sint64")) {
		type.BaseType.PlainOldDataType = IRPlainOldDataType::INT64;
		type.Signed = true;
	} else {
		throw std::logic_error("Unknown type: " + typeName);
	}
	return type;
}

IRType InstStructBuilder::BuildType(const gensim::isa::ISADescription* isa) const
{
	IRStructType &Struct = *(new IRStructType());
	Struct.Name = "Instruction";

	Struct.AddMember("Instr_Length", IRTypes::UInt8);
	Struct.AddMember("Instr_Code", IRTypes::UInt32);
	Struct.AddMember("ir", IRTypes::UInt32);
	Struct.AddMember("End_Of_Block", IRTypes::UInt8);
	Struct.AddMember("Uses_PC", IRTypes::UInt8);
	Struct.AddMember("IsPredicated", IRTypes::UInt8);
	Struct.AddMember("PredicateInfo", IRTypes::UInt32);
	Struct.AddMember("IsBranch", IRTypes::UInt8);

	const std::map<std::string, std::string> &fields = isa->Get_Disasm_Fields();
	for (std::map<std::string, std::string>::const_iterator ci = fields.begin(); ci != fields.end(); ++ci) {
		IRType FieldType = GetGenCType(ci->second);
		Struct.AddMember(ci->first, FieldType);
	}

	Struct.MakeFullyConst();

	IRType structType = IRType::CreateStruct(Struct);
	structType.Const = true;
	return structType;
}
