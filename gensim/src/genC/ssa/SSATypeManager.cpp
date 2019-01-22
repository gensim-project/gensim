/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSATypeManager.h"

using namespace gensim::genc::ssa;

SSATypeManager::SSATypeManager()
{
	InstallDefaultTypes();
}

SSATypeManager::~SSATypeManager()
{

}

bool SSATypeManager::InstallNamedType(const std::string& name, const IRType& type)
{
	if(HasNamedBasicType(name)) {
		throw std::logic_error("Already have a type named " + name);
	}

	type_map_[name] = type;
	return true;
}


bool SSATypeManager::InstallDefaultTypes()
{
	InstallNamedType("void", GetVoidType());

	InstallNamedType("uint8", GetIntegerType(8, false));
	InstallNamedType("uint16", GetIntegerType(16, false));
	InstallNamedType("uint32", GetIntegerType(32, false));
	InstallNamedType("uint64", GetIntegerType(64, false));
	InstallNamedType("uint128", GetIntegerType(128, false));

	InstallNamedType("sint8", GetIntegerType(8, true));
	InstallNamedType("sint16", GetIntegerType(16, true));
	InstallNamedType("sint32", GetIntegerType(32, true));
	InstallNamedType("sint64", GetIntegerType(64, true));
	InstallNamedType("sint128", GetIntegerType(128, true));

	InstallNamedType("float", GetFloat32());
	InstallNamedType("double", GetFloat64());

	return true;
}


const SSAType& SSATypeManager::GetIntegerType(uint8_t width, bool is_signed)
{
	switch (width) {
		case 1:
			if (is_signed) throw std::logic_error("Unsupported integer type width '" + std::to_string(width) + "'");
			return IRTypes::UInt1;

		case 8:
			if (is_signed) return IRTypes::Int8;
			else return IRTypes::UInt8;
		case 16:
			if (is_signed) return IRTypes::Int16;
			else return IRTypes::UInt16;
		case 32:
			if (is_signed) return IRTypes::Int32;
			else return IRTypes::UInt32;
		case 64:
			if (is_signed) return IRTypes::Int64;
			else return IRTypes::UInt64;
		case 128:
			if (is_signed) return IRTypes::Int128;
			else return IRTypes::UInt128;

		default:
			throw std::logic_error("Unsupported integer type width '" + std::to_string(width) + "'");
	}
}

const SSAType& SSATypeManager::GetFloatType(uint8_t width)
{
	switch (width) {
		case 32:
			return IRTypes::Float;
		case 64:
			return IRTypes::Double;

		default:
			throw std::logic_error("Unsupported floating-point type width '" + std::to_string(width) + "'");
	}
}

const SSAType& SSATypeManager::GetVoidType()
{
	return IRTypes::Void;
}

void SSATypeManager::InsertStructType(const std::string& name, const SSAType& struct_type)
{
	if(struct_types_.count(name)) {
		throw std::logic_error("Type manager already has a struct type called " + name);
	}
	struct_types_[name] = struct_type;
}

const SSAType& SSATypeManager::GetStructType(const std::string& name)
{
	return struct_types_.at(name);
}

bool SSATypeManager::HasStructType(const std::string& name) const
{
	return struct_types_.count(name);
}
