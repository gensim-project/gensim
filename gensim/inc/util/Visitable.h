/*
 * util/Visitable.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
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
