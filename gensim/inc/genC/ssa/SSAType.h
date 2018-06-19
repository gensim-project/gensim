/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include <genC/ir/IRType.h>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			typedef gensim::genc::IRType SSAType;

#if 0
			class SSATypeManager;
			namespace SSATypeKind
			{
				enum SSATypeKind {
					VOID,
					INTEGER,
					FLOAT,

					ARRAY,
					POINTER,
					REFERENCE,
					STRUCTURED,
					VECTOR,

					FUNCTION,
					BLOCK,
				};
			}

			class SSAType
			{
				friend class SSATypeManager;
			public:
				SSATypeKind::SSATypeKind GetKind() const
				{
					return _kind;
				}

				bool IsVoid() const
				{
					return _kind == SSATypeKind::VOID;
				}
				bool IsInteger() const
				{
					return _kind == SSATypeKind::INTEGER;
				}
				bool IsFloat() const
				{
					return _kind == SSATypeKind::FLOAT;
				}

				const SSAType& ScalarType() const;

				uint32_t SizeOfInBits() const;
				uint32_t SizeOfInBytes() const;

			private:
				SSAType(SSATypeKind::SSATypeKind kind) : _kind(kind) { }

				SSATypeKind::SSATypeKind _kind;
			};
#endif
		}
	}
}
