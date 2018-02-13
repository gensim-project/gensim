#include "genC/ssa/SSATypeManager.h"

using namespace gensim::genc::ssa;

SSATypeManager::SSATypeManager()
{

}

SSATypeManager::~SSATypeManager()
{

}

const SSAType& SSATypeManager::GetIntegerType(uint8_t width, bool is_signed)
{
	switch (width) {
		case 1:
			if (is_signed) throw std::logic_error("Unsupported integer type width '" + std::to_string(width) + "'");
			return _u1_type;

		case 8:
			if (is_signed) return _s8_type;
			else return _u8_type;
		case 16:
			if (is_signed) return _s16_type;
			else return _u16_type;
		case 32:
			if (is_signed) return _s32_type;
			else return _u32_type;
		case 64:
			if (is_signed) return _s64_type;
			else return _u64_type;

		default:
			throw std::logic_error("Unsupported integer type width '" + std::to_string(width) + "'");
	}
}

const SSAType& SSATypeManager::GetFloatType(uint8_t width)
{
	switch (width) {
		case 32:
			return _f32_type;
		case 64:
			return _f64_type;

		default:
			throw std::logic_error("Unsupported floating-point type width '" + std::to_string(width) + "'");
	}
}

const SSAType& SSATypeManager::GetVoidType()
{
	return _void_type;
}

void SSATypeManager::InsertStructType(const std::string& name, const SSAType& struct_type)
{
	struct_types_[name] = struct_type;
}

const SSAType& SSATypeManager::GetStructType(const std::string& name)
{
	return struct_types_.at(name);
}
