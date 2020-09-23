/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   IRType.h
 * Author: s0803652
 *
 * Created on 31 July 2012, 16:38
 */

#ifndef IRTYPE_H
#define IRTYPE_H

#include <cassert>

#include <sstream>
#include <stdint.h>
#include <vector>

#include "../../define.h"
#include "IREnums.h"

namespace gensim
{
	namespace genc
	{

		class IRStructType;

		class IRTypes;
		class IRConstant;

		class IRType
		{
			friend class IRTypes;

		public:
			enum IRDataType {
				PlainOldData,
				Struct,
				// TEMPORARY TYPES FOR MIGRATION
				Block,
				Function
			};

			enum PromoteResult {
				PROMOTE_OK,
				PROMOTE_OK_SIGNED,
				PROMOTE_SIGN_CHANGE,
				PROMOTE_TRUNCATE,
				PROMOTE_CONVERT,
				PROMOTE_TO_STRUCT,
				PROMOTE_FROM_STRUCT,
				PROMOTE_VECTOR
			};

			static bool ParseType(std::string text, IRType &out);
			static IRType Ref(const IRType &BaseType);
			static IRType CreateStruct(const IRStructType * const Type);
			static const IRType Resolve(BinaryOperator::EBinaryOperator op, const IRType &LHS, const IRType &RHS);
			static const IRType &GetIntType(uint8_t width);

			static IRConstant Cast(const IRConstant &value, const IRType &from, const IRType &to);

			union {
				IRPlainOldDataType::IRPlainOldDataType PlainOldDataType;
				const IRStructType *StructType;
			} BaseType;

			IRDataType DataType;
			uint8_t VectorWidth;
			bool Signed;
			bool Reference;
			bool Const;

			/**
			 * Get the size of this type in bytes
			 */
			uint32_t SizeInBytes() const;

			/**
			 * If this is a vector, return the size of a single element. Otherwise, return the total size.
			 */
			uint32_t ElementSize() const;

			/**
			 * Get the smallest C type which can hold this type
			 */
			std::string GetCType() const;

			/**
			 * Get the smallest LLVM datatype which can hold this type
			 */
			std::string GetLLVMType() const;

			/**
			 * Get the smallest unified datatype descriptor which can hold this type
			 */
			std::string GetUnifiedType() const;

			/**
			 * Get the smallest Captive datatype which can hold this type
			 */
			std::string GetCaptiveType() const;

			/**
			 * If this is an integer type, return the maximum value it can store
			 */
			uint64_t GetMaxValue() const;

			/**
			 * If this is a vector type, return the element type
			 */
			IRType GetElementType() const;

			void PrettyPrint(std::ostringstream &out) const;
			std::string PrettyPrint() const;

			/**
			 * Attempt to automatically promote this type to PromoteTo. If the promotion is allowed,
			 * return PROMOTE_OK, otherwise return an enum representing the problem.
			 * @param PromoteTo The type to attempt to promote this type to.
			 * @return The effect of the promotion
			 */
			PromoteResult AutoPromote(const IRType &PromoteTo) const;

			bool IsInteger() const
			{
				return DataType == PlainOldData && BaseType.PlainOldDataType > IRPlainOldDataType::VOID && BaseType.PlainOldDataType < IRPlainOldDataType::FLOAT;
			}

			bool IsFloating() const
			{
				return DataType == PlainOldData && BaseType.PlainOldDataType >= IRPlainOldDataType::FLOAT;
			}

			bool IsStruct() const
			{
				return DataType == Struct;
			}

			bool operator==(const IRType &other) const
			{
				if (DataType != other.DataType) return false;
				if (Signed != other.Signed) return false;
				if (VectorWidth != other.VectorWidth) return false;
				switch (DataType) {
					case PlainOldData:
						return BaseType.PlainOldDataType == other.BaseType.PlainOldDataType;
					case Struct:
						// Since there is only one declaration of each struct type, there can only be one pointer to each struct type.
						return (BaseType.StructType) == (other.BaseType.StructType);
					case Block:
					case Function:
						UNIMPLEMENTED;
				}
				UNEXPECTED;
			}

			bool operator!=(const IRType &other) const
			{
				return !(*this == other);
			}

			IRType() : DataType(PlainOldData), Signed(false), Reference(false), Const(false), VectorWidth(1)
			{
				BaseType.PlainOldDataType = IRPlainOldDataType::VOID;
			}

			static IRType Vector(const IRType &base, int vectorwidth)
			{
				IRType b = base;
				b.VectorWidth = vectorwidth;
				return b;
			}

		private:
			static IRType _UInt1(), _Int8(), _Int16(), _Int32(), _Int64(), _Int128(), _UInt8(), _UInt16(), _UInt32(), _UInt64(), _UInt128(), _Void(), _Float(), _Double(), _LongDouble(), _Block(), _Function();
		};

		class IRTypes
		{
		public:
			static const IRType UInt1, Int8, Int16, Int32, Int64, Int128, UInt8, UInt16, UInt32, UInt64, UInt128, Void, Float, Double, LongDouble, Block, Function;
		};

		class IRStructType
		{
		public:
			class IRStructMember
			{
			public:
				std::string Name;
				IRType Type;

				IRStructMember(std::string name, IRType type) : Name(name), Type(type) {}
			};

			std::string Name;
			std::vector<IRStructMember> Members;

			void AddMember(std::string, const IRType &Type);
			bool HasMember(std::string) const;
			uint32_t GetOffset(std::string) const;
			const IRType &GetMemberType(std::string) const;

			void MakeFullyConst()
			{
				for (std::vector<IRStructMember>::iterator i = Members.begin(); i != Members.end(); ++i) {
					i->Type.Const = true;
				}
			}
		};
	}
}
#endif /* IRTYPE_H */
