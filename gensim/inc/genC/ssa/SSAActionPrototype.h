/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "genC/ir/IRSignature.h"
#include "genC/ssa/SSAType.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAActionPrototype
			{
			public:
				explicit SSAActionPrototype(const IRSignature& signature);

				const IRSignature& GetIRSignature() const
				{
					return _signature;
				}

				const SSAType& ReturnType() const
				{
					return _return_type;
				}
				const std::vector<SSAType>& ParameterTypes() const
				{
					return _parameter_types;
				}

				bool HasAttribute(ActionAttribute::EActionAttribute attribute) const
				{
					return _signature.HasAttribute(attribute);
				}
			private:
				IRSignature _signature;

				SSAType _return_type;
				std::vector<SSAType> _parameter_types;
			};
		}
	}
}
