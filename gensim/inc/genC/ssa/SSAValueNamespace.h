/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#define FMT_VN PRIu64

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAValueNamespace
			{
			public:
				typedef uint64_t value_name_t;

				SSAValueNamespace() : _next_name(0) {}

				value_name_t GetName()
				{
					return _next_name++;
				}

			private:
				value_name_t _next_name;
			};
		}
	}
}
