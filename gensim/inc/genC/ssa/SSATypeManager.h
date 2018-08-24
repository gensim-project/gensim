/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "SSAType.h"

#include <map>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSATypeManager
			{
			public:
				SSATypeManager();
				~SSATypeManager();

				const SSAType& GetVoidType();
				const SSAType& GetIntegerType(uint8_t width, bool is_signed = false);
				const SSAType& GetFloatType(uint8_t width);
				const SSAType& GetPtrType(const SSAType& underlying_type);
				const SSAType& GetVectorType(const SSAType& underlying_type, uint8_t width);

				const SSAType &GetStructType(const std::string &name);
				bool HasStructType(const std::string &name) const;
				void InsertStructType(const std::string &name, const SSAType &struct_type);

				const SSAType& GetUInt8()
				{
					return GetIntegerType(8, false);
				}
				const SSAType& GetUInt16()
				{
					return GetIntegerType(16, false);
				}
				const SSAType& GetUInt32()
				{
					return GetIntegerType(32, false);
				}
				const SSAType& GetUInt64()
				{
					return GetIntegerType(64, false);
				}
				const SSAType& GetSInt8()
				{
					return GetIntegerType(8, true);
				}
				const SSAType& GetSInt16()
				{
					return GetIntegerType(16, true);
				}
				const SSAType& GetSInt32()
				{
					return GetIntegerType(32, true);
				}
				const SSAType& GetSInt64()
				{
					return GetIntegerType(64, true);
				}

				const SSAType& GetFloat32()
				{
					return GetFloatType(32);
				}
				const SSAType& GetFloat64()
				{
					return GetFloatType(64);
				}

			private:
				SSATypeManager(const SSATypeManager&) = delete;
				SSATypeManager(SSATypeManager&&) = delete;

				SSAType _void_type;
				SSAType _u1_type, _u8_type, _u16_type, _u32_type, _u64_type;
				SSAType _s8_type, _s16_type, _s32_type, _s64_type;
				SSAType _f32_type, _f64_type;

				std::map<std::string, SSAType> struct_types_;
			};
		}
	}
}
