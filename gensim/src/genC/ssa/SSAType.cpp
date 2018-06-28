/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <genC/ssa/SSAType.h>

#if 0
#include <genC/ir/IRSignature.h>

using namespace gensim::genc::ssa;

SSAType SSAPrimitiveType::vd(new SSAPrimitiveType(PrimitiveTypeKind::VOID));
SSAType SSAPrimitiveType::u1(new SSAPrimitiveType(PrimitiveTypeKind::U1));
SSAType SSAPrimitiveType::u8(new SSAPrimitiveType(PrimitiveTypeKind::U8));
SSAType SSAPrimitiveType::u16(new SSAPrimitiveType(PrimitiveTypeKind::U16));
SSAType SSAPrimitiveType::u32(new SSAPrimitiveType(PrimitiveTypeKind::U32));
SSAType SSAPrimitiveType::u64(new SSAPrimitiveType(PrimitiveTypeKind::U64));
SSAType SSAPrimitiveType::s8(new SSAPrimitiveType(PrimitiveTypeKind::S8));
SSAType SSAPrimitiveType::s16(new SSAPrimitiveType(PrimitiveTypeKind::S16));
SSAType SSAPrimitiveType::s32(new SSAPrimitiveType(PrimitiveTypeKind::S32));
SSAType SSAPrimitiveType::s64(new SSAPrimitiveType(PrimitiveTypeKind::S64));
SSAType SSAPrimitiveType::f32(new SSAPrimitiveType(PrimitiveTypeKind::F32));
SSAType SSAPrimitiveType::f64(new SSAPrimitiveType(PrimitiveTypeKind::F64));
SSAType SSABlockType::BlockType(new SSABlockType());

/**
 * Constructs an SSAType based on an IR Plain-old-data type specifier, with a potential
 * vector width.
 * @param pod The plain-old-data type specifier.
 * @param is_signed Whether or not this POD type is signed.
 * @param vector_width Specifies the vector width, if this is a vector type.
 * @return Returns an SSAType representing this POD type, potentially encapsulated
 * in a vector representation.
 */
SSAType SSATypeHelper::FromPlainOldIRType(genc::IRPlainOldDataType::IRPlainOldDataType pod, bool is_signed, int vector_width)
{
	assert(vector_width > 0);

	// TODO: Think about this.  Is it right to assume a vector width of one is not
	// a vector at all?
	if (vector_width == 1) return FromPlainOldIRType(pod, is_signed);
	else return SSAType(new SSAVectorType(FromPlainOldIRType(pod, is_signed), vector_width));
}

/**
 * Constructs an SSAType based on an IR Plain-old-data type specifier.
 * @param pod The plain-old-data type specifier.
 * @param is_signed Whether or not this POD type is signed.
 * @return Returns an SSAType representing this POD type.
 */
SSAType SSATypeHelper::FromPlainOldIRType(genc::IRPlainOldDataType::IRPlainOldDataType pod, bool is_signed)
{
	switch (pod) {
		case IRPlainOldDataType::VOID:
			return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::VOID));

		case IRPlainOldDataType::INT1:
			if (is_signed)
				throw std::logic_error("invalid IR type");

			return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::U1));

		case IRPlainOldDataType::INT8:
			if (is_signed) return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::S8));
			else return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::U8));
		case IRPlainOldDataType::INT16:
			if (is_signed) return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::S16));
			else return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::U16));
		case IRPlainOldDataType::INT32:
			if (is_signed) return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::S32));
			else return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::U32));
		case IRPlainOldDataType::INT64:
			if (is_signed) return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::S64));
			else return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::U64));

		case IRPlainOldDataType::FLOAT:
			return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::F32));
		case IRPlainOldDataType::DOUBLE:
			return SSAType(new SSAPrimitiveType(PrimitiveTypeKind::F64));

		default:
			throw std::logic_error("Unsupported plain-old data type");
	}
}

/**
 * Constructs an SSAType based on the IRType.
 * @param type The IRType to base the new SSAType off of.
 * @return Returns the SSAType that represents the incoming IRType.
 */
SSAType SSATypeHelper::FromIRType(const IRType& type)
{
	// Don't support non-pod types (yet?)
	if (type.DataType != IRType::PlainOldData) {
		throw std::logic_error("Non-plain-old-data IR types are not supported.");
	}

	// Don't support indirectness (yet?)
	if (type.Indirectness > 0) {
		throw std::logic_error("IR types with indirectness are not supported.");
	}

	// If the type is a reference, return an encapsulated reference type.
	if (type.Reference) {
		return SSAType(new SSAReferenceType(FromPlainOldIRType(type.BaseType.PlainOldDataType, type.Signed, type.VectorWidth)));
	} else {
		return FromPlainOldIRType(type.BaseType.PlainOldDataType, type.Signed, type.VectorWidth);
	}
}

/**
 * Constructs an SSAFunctionType from an IRSignature.
 * @param sig The IRSignature to generate the SSAFunctionType from.
 * @return returns an SSAFunctionType that represents the given IRSignature.
 */
SSAType SSAFunctionType::FromIRSignature(const IRSignature& sig)
{
	// Build a vector of SSATypes that represents the types of the parameters
	// in the IRSignature.
	std::vector<SSAType> parameter_types;
	for(const auto& param : sig.GetParams()) {
		parameter_types.push_back(SSATypeHelper::FromIRType(param.GetType()));
	}

	// Return a new SSAFunctionType that has the specified return type and parameter types.
	return SSAType(new SSAFunctionType(SSATypeHelper::FromIRType(sig.GetType()), parameter_types));
}

/**
 * Default pretty-printer for an SSA type.
 * @return Returns a string representation of the type.
 */
std::string _SSAType::PrettyPrint() const
{
	return "<unknown ssa type>";
}

std::string SSAReferenceType::PrettyPrint() const
{
	return _underlying_type->PrettyPrint() + "&";
}

std::string SSAPrimitiveType::PrettyPrint() const
{
	switch (_kind) {
		case PrimitiveTypeKind::VOID:
			return "void";

		case PrimitiveTypeKind::U1:
			return "u1";
		case PrimitiveTypeKind::U8:
			return "u8";
		case PrimitiveTypeKind::U16:
			return "u16";
		case PrimitiveTypeKind::U32:
			return "u32";
		case PrimitiveTypeKind::U64:
			return "u64";

		case PrimitiveTypeKind::S8:
			return "s8";
		case PrimitiveTypeKind::S16:
			return "s16";
		case PrimitiveTypeKind::S32:
			return "s32";
		case PrimitiveTypeKind::S64:
			return "s64";

		case PrimitiveTypeKind::F32:
			return "f32";
		case PrimitiveTypeKind::F64:
			return "f64";

		default:
			return "???";
	}
}

std::string SSAFunctionType::PrettyPrint() const
{
	std::string param_str;

	bool first = true;
	for(const auto& type : _parameter_types) {
		if (first) {
			first = false;
		} else {
			param_str += ", ";
		}

		param_str += type->PrettyPrint();
	}

	return _return_type->PrettyPrint() + " (" + param_str + ")";
}

SSABlockType::SSABlockType()
{

}

ConversionResult::ConversionResult _SSAType::CheckConversion(const SSAType& target) const
{
	assert(false && "TODO");
	return ConversionResult::PROMOTION_FAILED;
}

uint64_t _SSAType::PerformCast(uint64_t value, const SSAType& to) const
{
	assert(false && "TODO");
	return value;
}


bool SSAPrimitiveType::IsSigned() const
{
	switch (_kind) {
		case PrimitiveTypeKind::S8:
		case PrimitiveTypeKind::S16:
		case PrimitiveTypeKind::S32:
		case PrimitiveTypeKind::S64:
		case PrimitiveTypeKind::F32:
		case PrimitiveTypeKind::F64:
			return true;
		default:
			return false;
	}
}

bool SSAPrimitiveType::IsFloatingPoint() const
{
	switch (_kind) {
		case PrimitiveTypeKind::F32:
		case PrimitiveTypeKind::F64:
			return true;
		default:
			return false;
	}
}

uint8_t SSAPrimitiveType::SizeOf() const
{
	switch (_kind) {
		case PrimitiveTypeKind::VOID:
			return 0;

		case PrimitiveTypeKind::U1:
			return 1;

		case PrimitiveTypeKind::S8:
		case PrimitiveTypeKind::U8:
			return 1;

		case PrimitiveTypeKind::S16:
		case PrimitiveTypeKind::U16:
			return 2;

		case PrimitiveTypeKind::S32:
		case PrimitiveTypeKind::U32:
		case PrimitiveTypeKind::F32:
			return 4;

		case PrimitiveTypeKind::S64:
		case PrimitiveTypeKind::U64:
		case PrimitiveTypeKind::F64:
			return 8;

		default:
			return 0;
	}
}

SSAType SSAReferenceType::MakeReference(const SSAType& to)
{
	return SSAType(new SSAReferenceType(to));
}

std::string SSATypeHelper::AsCType(const SSAType& ssa_type)
{
	if (auto pt = ssa_type.As<SSAPrimitiveType>()) {
		switch (pt->GetKind()) {
			case PrimitiveTypeKind::F32:
				return "float";
			case PrimitiveTypeKind::F64:
				return "double";
			case PrimitiveTypeKind::U1:
				return "bool";
			case PrimitiveTypeKind::U8:
				return "unsigned char";
			case PrimitiveTypeKind::U16:
				return "unsigned short";
			case PrimitiveTypeKind::U32:
				return "unsigned int";
			case PrimitiveTypeKind::U64:
				return "unsigned long long int";
			case PrimitiveTypeKind::S8:
				return "signed char";
			case PrimitiveTypeKind::S16:
				return "signed short";
			case PrimitiveTypeKind::S32:
				return "signed int";
			case PrimitiveTypeKind::S64:
				return "signed long long int";
			case PrimitiveTypeKind::VOID:
				return "void";
		}
	} else if (auto rt = ssa_type.As<SSAReferenceType>()) {
		return AsCType(rt->GetUnderlyingType()) + "&";
	} else {
		throw std::logic_error("not implemented");
	}
}

std::string SSATypeHelper::AsLLVMType(const SSAType& ssa_type)
{
	return "???";
}

SSAVectorType::~SSAVectorType()
{

}

std::string SSAVectorType::PrettyPrint() const
{
	return "X";
}
#endif
