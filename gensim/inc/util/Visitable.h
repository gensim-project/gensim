/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

namespace gensim
{
	namespace util
	{
		template<class VisitorType>
		class Visitable
		{
		public:
			virtual ~Visitable() { }
			virtual void Accept(VisitorType& visitor) = 0;
		};
	}
}
