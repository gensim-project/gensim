
#include <cassert>
#include <string>

#include "define.h"
#include "genC/ir/IRType.h"
#include "genC/ir/IR.h"

#ifdef MAX
#undef MAX
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

namespace gensim
{
	namespace genc
	{

		IRType IRType::_UInt128()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT128;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Int128()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT128;
			g.Signed = true;
			g.Const = false;
			return g;
		}
		
		IRType IRType::_UInt64()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT64;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Int64()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT64;
			g.Signed = true;
			g.Const = false;
			return g;
		}

		IRType IRType::_UInt32()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT32;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Int32()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT32;
			g.Signed = true;
			g.Const = false;
			return g;
		}

		IRType IRType::_UInt16()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT16;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Int16()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT16;
			g.Signed = true;
			g.Const = false;
			return g;
		}

		IRType IRType::_UInt8()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT8;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Int8()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT8;
			g.Signed = true;
			g.Const = false;
			return g;
		}

		IRType IRType::_UInt1()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::INT1;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Float()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::FLOAT;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Double()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::DOUBLE;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_LongDouble()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::LONG_DOUBLE;
			g.Signed = false;
			g.Const = false;
			return g;
		}

		IRType IRType::_Void()
		{
			IRType g;
			g.DataType = IRType::PlainOldData;
			g.BaseType.PlainOldDataType = IRPlainOldDataType::VOID;
			g.Signed = false;
			g.Const = true;
			return g;
		}

		IRType IRType::_Block()
		{
			IRType g;
			g.DataType = IRType::Block;
			return g;
		}

		IRType IRType::_Function()
		{
			IRType g;
			g.DataType = IRType::Function;
			return g;
		}

		const IRType IRTypes::Int8 = IRType::_Int8();
		const IRType IRTypes::Int16 = IRType::_Int16();
		const IRType IRTypes::Int32 = IRType::_Int32();
		const IRType IRTypes::Int64 = IRType::_Int64();
		const IRType IRTypes::Int128 = IRType::_Int128();
		const IRType IRTypes::UInt1 = IRType::_UInt1();
		const IRType IRTypes::UInt8 = IRType::_UInt8();
		const IRType IRTypes::UInt16 = IRType::_UInt16();
		const IRType IRTypes::UInt32 = IRType::_UInt32();
		const IRType IRTypes::UInt64 = IRType::_UInt64();
		const IRType IRTypes::UInt128 = IRType::_UInt128();
		const IRType IRTypes::Void = IRType::_Void();
		const IRType IRTypes::Float = IRType::_Float();
		const IRType IRTypes::Double = IRType::_Double();
		const IRType IRTypes::LongDouble = IRType::_LongDouble();
		const IRType IRTypes::Block = IRType::_Block();
		const IRType IRTypes::Function = IRType::_Function();

		IRType IRType::CreateStruct(const IRStructType &Type)
		{
			IRType g;
			g.DataType = Struct;
			g.BaseType.StructType = &Type;
			return g;
		}


		IRType IRType::Ref(const IRType &BaseType)
		{
			IRType newtype = BaseType;
			newtype.Reference = true;
			return newtype;
		}

		IRType::PromoteResult IRType::AutoPromote(const IRType &PromoteTo) const
		{
			if (DataType != PromoteTo.DataType) return DataType == PlainOldData ? PROMOTE_TO_STRUCT : PROMOTE_FROM_STRUCT;
			if (VectorWidth != PromoteTo.VectorWidth) return PROMOTE_VECTOR;
			if (IsFloating() != PromoteTo.IsFloating()) return PROMOTE_CONVERT;
			if (IsFloating() && DataType != PromoteTo.DataType) return PROMOTE_CONVERT;
			if (BaseType.PlainOldDataType > PromoteTo.BaseType.PlainOldDataType) return PROMOTE_TRUNCATE;
			if (Signed != PromoteTo.Signed) return PROMOTE_SIGN_CHANGE;

			if (Signed) return PROMOTE_OK_SIGNED;
			return PROMOTE_OK;
		}

		const IRType &IRType::GetIntType(uint8_t width_in_bits)
		{
			switch (width_in_bits) {
				case 8:
					return IRTypes::UInt8;
				case 16:
					return IRTypes::UInt16;
				case 32:
					return IRTypes::UInt32;
				case 64:
					return IRTypes::UInt64;
				case 128:
					return IRTypes::UInt128;
			}
			throw std::logic_error("Unsupported integer type");
		}

		IRConstant IRType::Cast(const IRConstant &value, const IRType& from, const IRType& to)
		{
			// are we doing a vsplat?
			if((from.VectorWidth == 1) && (to.VectorWidth > 1)) {
				return IRConstant::Vector(to.VectorWidth, value);
			}
			
			if((from.VectorWidth != 1) || (to.VectorWidth != 1)) {
				GASSERT(value.Type() == IRConstant::Type_Vector);
				GASSERT(from.VectorWidth == to.VectorWidth);
				GASSERT(from.GetElementType() == to.GetElementType());

				IRConstant newval = value;
				if(from == to) {
					for(unsigned i = 0; i < from.VectorWidth; ++i) {
						newval.VPut(i, Cast(newval.VGet(i), from.GetElementType(), to.GetElementType()));
					}
				} else {
					UNIMPLEMENTED;
				}

				return newval;
			}

			if(from.IsFloating() || to.IsFloating()) {
				// 3 types of cast: cast to float, cast from float, change float type
				// cast to float
				if(!from.IsFloating() && to.IsFloating()) {
					if(to.BaseType.PlainOldDataType == IRPlainOldDataType::FLOAT) {
						return IRConstant::Float(value.Int());
					} else if(to.BaseType.PlainOldDataType == IRPlainOldDataType::DOUBLE) {
						return IRConstant::Double(value.Int());
					} else if(to.BaseType.PlainOldDataType == IRPlainOldDataType::LONG_DOUBLE) {
						return IRConstant::LongDouble(value.Int());
					} else {
						UNIMPLEMENTED;
					}
				}
				// cast from float
				if(from.IsFloating() && !to.IsFloating()) {
					if(from.BaseType.PlainOldDataType == IRPlainOldDataType::FLOAT) {
						return IRConstant::Integer(value.Flt());
					} else if(from.BaseType.PlainOldDataType == IRPlainOldDataType::DOUBLE) {
						return IRConstant::Integer(value.Dbl());
					} else {
						UNIMPLEMENTED;
					}
				}
				// change float type
				if(from.IsFloating() && to.IsFloating()) {
					if(from.BaseType.PlainOldDataType == to.BaseType.PlainOldDataType) {
						return value;
					}

					if(from.BaseType.PlainOldDataType == IRPlainOldDataType::FLOAT) {
						// cast from float to double
						return IRConstant::Double(value.Flt());
					}
					if(from.BaseType.PlainOldDataType == IRPlainOldDataType::DOUBLE) {
						// cast from double to float
						return IRConstant::Float(value.Dbl());
					}

					UNIMPLEMENTED;

				}
			}

			uint64_t cast_value;
			IRConstant result = value;

			if(from.Signed && to.Signed) {
				if (from.Size() == 16 || to.Signed == 16) {
					throw std::logic_error("Sign extension to/from 128-bit integer not supported");
				}
				
				// sign extend from value to 64 bits
				switch(from.Size()) {
					case 1:
						result = IRConstant::Integer((int64_t)(int8_t)value.Int());
						break;
					case 2:
						result = IRConstant::Integer((int64_t)(int16_t)value.Int());
						break;
					case 4:
						result = IRConstant::Integer((int64_t)(int32_t)value.Int());
						break;
					case 8:
						result = IRConstant::Integer((int64_t)(int64_t)value.Int());
						break;
					default:
						UNEXPECTED;
				}
			}

			if(to.BaseType.PlainOldDataType == IRPlainOldDataType::INT64) {
				uint64_t result;
				switch(value.Type()) {
					case IRConstant::Type_Integer:
						result = value.Int();
						break;
					case IRConstant::Type_Float_Single:
						result = value.Flt();
						break;
					case IRConstant::Type_Float_Double:
						result = value.Dbl();
						break;
					default:
						UNEXPECTED;
				}
				return IRConstant::Integer(result);
			}

			uint64_t mask = (1ULL << (to.Size()*8))-1;
			cast_value = result.Int() & mask;

			return IRConstant::Integer(cast_value);
		}


		bool IRType::ParseType(std::string text, IRType &type)
		{
			uint8_t annotations = text.find('*');
			std::string baseType = text.substr(0, annotations);

			if (baseType == "uint8") {
				type = IRTypes::UInt8;
			} else if (baseType == "sint8") {
				type = IRTypes::Int8;
			} else if (baseType == "uint16") {
				type = IRTypes::UInt16;
			} else if (baseType == "sint16") {
				type = IRTypes::Int16;
			} else if (baseType == "uint32") {
				type = IRTypes::UInt32;
			} else if (baseType == "sint32") {
				type = IRTypes::Int32;
			} else if (baseType == "uint64") {
				type = IRTypes::UInt64;
			} else if (baseType == "sint64") {
				type = IRTypes::Int64;
			} else if (baseType == "uint128") {
				type = IRTypes::UInt128;
			} else if (baseType == "sint128") {
				type = IRTypes::Int128;
			} else if (baseType == "float") {
				type = IRTypes::Float;
			} else if (baseType == "double") {
				type = IRTypes::Double;
			} else if (baseType == "void") {
				type = IRTypes::Void;
			} else {
				return false;
			}


			return true;
		}

		const IRType IRType::Resolve(BinaryOperator::EBinaryOperator op, const IRType &LHS, const IRType &RHS)
		{
			// Actual arithmetic operations do complex resolution
			if (op < BinaryOperator::END_OF_NORMAL_OPERATORS) {

				IRType Out;

				Out.Const = LHS.Const && RHS.Const;
				Out.Signed = LHS.Signed && RHS.Signed;
				Out.VectorWidth = LHS.VectorWidth;

				if (LHS.DataType == Struct && RHS.DataType == Struct) {
					Out.DataType = Struct;
					Out.BaseType.StructType = LHS.BaseType.StructType;
				} else if (LHS.DataType == Struct || RHS.DataType == Struct)
					return IRTypes::Void;
				else
					Out.BaseType.PlainOldDataType = MAX(LHS.BaseType.PlainOldDataType, RHS.BaseType.PlainOldDataType);

				if(Out.IsFloating()) {
					Out.Signed = false;
				}

				return Out;
			} else {
				// comparison operators return uint8s ('bool's), unless they're a comparison of a vector type, in
				// which case a vector of the same type is returned.
				if (LHS.VectorWidth > 1) {
					// The vector width must be the same, or the RHS must be scalar.
					if (LHS.VectorWidth == RHS.VectorWidth || RHS.VectorWidth == 1) {
						return LHS;
					} else {
						throw std::logic_error("comparison of vectors with different width");
					}
				} else {
					return IRTypes::UInt8;
				}
			}
		}
		uint32_t IRType::Size() const
		{
			assert(VectorWidth != 0);
			return VectorWidth * ElementSize();
		}

		uint32_t IRType::ElementSize() const
		{
			if (DataType == IRType::PlainOldData) {
				switch (BaseType.PlainOldDataType) {
					case IRPlainOldDataType::VOID:
						return 0;
					case IRPlainOldDataType::INT1:
						return 1;
					case IRPlainOldDataType::INT8:
						return 1;
					case IRPlainOldDataType::INT16:
						return 2;
					case IRPlainOldDataType::INT32:
						return 4;
					case IRPlainOldDataType::INT64:
						return 8;
					case IRPlainOldDataType::INT128:
						return 16;
					case IRPlainOldDataType::FLOAT:
						return 4;
					case IRPlainOldDataType::DOUBLE:
						return 8;
					case IRPlainOldDataType::LONG_DOUBLE:
						return 10;
					default:
						assert(false && "Unrecognized type");
				}
				return 0;
			} else {
				const IRStructType &t = *BaseType.StructType;
				uint32_t size = 0;
				for (std::vector<IRStructType::IRStructMember>::const_iterator ci = t.Members.begin(); ci != t.Members.end(); ++ci) {
					size += ci->Type.Size();
				}
				return size;
			}
		}

		std::string IRType::GetCType() const
		{
			std::ostringstream out;
			if (DataType == IRType::PlainOldData) {
				if(VectorWidth > 1) {
					IRType innertype = *this;
					innertype.VectorWidth = 1;

					out << "Vector<" << innertype.GetCType() << ", " << (uint32_t)VectorWidth << ">";
					return out.str();
				}

				if (BaseType.PlainOldDataType == IRPlainOldDataType::VOID)
					out << "void";
				else if (BaseType.PlainOldDataType == IRPlainOldDataType::INT1)
					out << "bool";
				else {
					if (BaseType.PlainOldDataType < IRPlainOldDataType::FLOAT) {
						if (Signed)
							out << "";
						else
							out << "u";
					}
					switch (BaseType.PlainOldDataType) {
						case IRPlainOldDataType::INT8:
							out << "int8_t";
							break;
						case IRPlainOldDataType::INT16:
							out << "int16_t";
							break;
						case IRPlainOldDataType::INT32:
							out << "int32_t";
							break;
						case IRPlainOldDataType::INT64:
							out << "int64_t";
							break;
						case IRPlainOldDataType::INT128:
							out << "int128_t";
							break;
						case IRPlainOldDataType::FLOAT:
							out << "float";
							break;
						case IRPlainOldDataType::DOUBLE:
							out << "double";
							break;
						case IRPlainOldDataType::LONG_DOUBLE:
							out << "long double";
							break;
						default:
							assert(false && "Unrecognized data type");
					}
				}
			}
			// we treat structs as pointers to structs. since we are just doing pointer arithmetic we
			// don't care what type it actually is, so return a char pointer
			else
				return "uint8*";

			if (Reference) out << "&";
			return out.str();
		}

		std::string IRType::GetLLVMType() const
		{
			assert(DataType == PlainOldData && "Can only get llvm types of POD.");
			std::string llvm_type;
			switch (BaseType.PlainOldDataType) {
				case IRPlainOldDataType::VOID:
					assert(false && "Attempting to lower a void value for some reason");
					return "???";
				case IRPlainOldDataType::INT1:
					llvm_type = "txln_ctx.types.i1";
					break;
				case IRPlainOldDataType::INT8:
					llvm_type = "txln_ctx.types.i8";
					break;
				case IRPlainOldDataType::INT16:
					llvm_type = "txln_ctx.types.i16";
					break;
				case IRPlainOldDataType::INT32:
					llvm_type = "txln_ctx.types.i32";
					break;
				case IRPlainOldDataType::INT64:
					llvm_type = "txln_ctx.types.i64";
					break;
				case IRPlainOldDataType::FLOAT:
					llvm_type = "txln_ctx.types.f32";
					break;
				case IRPlainOldDataType::DOUBLE:
					llvm_type = "txln_ctx.types.f64";
					break;
				default:
					assert(false && "Unrecognized type");
			}


			std::ostringstream str;
			if(VectorWidth != 1) {
				str << "llvm::VectorType::get(" << llvm_type << ", " << (uint32_t)VectorWidth << ")";
			} else {
				str << llvm_type;
			}

			return str.str();
		}

		std::string IRType::GetUnifiedType() const
		{
			if (IsStruct()) {
				return "u64";
			} else if (VectorWidth == 1) {
				if (IsFloating()) {
					if (BaseType.PlainOldDataType == IRPlainOldDataType::FLOAT)
						return "f32";
					else if (BaseType.PlainOldDataType == IRPlainOldDataType::DOUBLE)
						return "f64";
					else if (BaseType.PlainOldDataType == IRPlainOldDataType::LONG_DOUBLE)
						return "f80";
				} else {
					if (Signed) {
						switch (Size()) {
							case 1:
								return "s8";
							case 2:
								return "s16";
							case 4:
								return "s32";
							case 8:
								return "s64";
							case 16:
								return "s128";
						}
					} else {
						switch (Size()) {
							case 1:
								return "u8";
							case 2:
								return "u16";
							case 4:
								return "u32";
							case 8:
								return "u64";
							case 16:
								return "u128";
						}
					}
				}
			} else {
				return "v" + std::to_string(VectorWidth) + GetElementType().GetUnifiedType();
			}

			return "XXX--" + GetCType() + "--XXX";
		}

		std::string IRType::GetCaptiveType() const
		{
			if (VectorWidth > 1) {
				return "IRDataType::vector(" + GetElementType().GetCaptiveType() + ", " + std::to_string(VectorWidth) + ")";
			} else {
				return "IRDataType::" + GetUnifiedType() + "()";
			}
		}

		uint64_t IRType::GetMaxValue() const
		{
			assert(DataType == PlainOldData);
			assert(!IsFloating());

			return 1 << (8*Size()) - 1;
		}

		IRType IRType::GetElementType() const
		{
			IRType rval = *this;
			rval.VectorWidth = 1;
			return rval;
		}

		void IRStructType::AddMember(std::string Name, const IRType &Type)
		{
			Members.push_back(IRStructMember(Name, Type));
		}

		bool IRStructType::HasMember(std::string Member) const
		{
			for (std::vector<IRStructMember>::const_iterator ci = Members.begin(); ci != Members.end(); ++ci) {
				if (ci->Name == Member) return true;
			}
			return false;
		}

		uint32_t IRStructType::GetOffset(std::string member) const
		{
			uint32_t offset = 0;
			for (std::vector<IRStructMember>::const_iterator ci = Members.begin(); ci != Members.end(); ++ci) {
				if (ci->Name == member) return offset;
				offset += ci->Type.Size();
			}
			assert(false && "Could not find struct member");
			return 0;
		}

		const IRType &IRStructType::GetMemberType(std::string Member) const
		{
			for (std::vector<IRStructMember>::const_iterator ci = Members.begin(); ci != Members.end(); ++ci) {
				if (ci->Name == Member) return ci->Type;
			}
			assert(false && "Could not find type");
			return IRTypes::Void;
		}
	}
}
