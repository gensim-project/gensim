/*
 * util/Disposable.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include <stdexcept>

namespace gensim
{
	namespace util
	{
		class Disposable
		{
		public:
			Disposable() : disposed_(false), dispose_addr_(nullptr) { }
			virtual ~Disposable() { }

			bool IsDisposed() const
			{
				return disposed_;
			}

			void Dispose()
			{
				CheckDisposal();
				PerformDispose();

				disposed_ = true;
				dispose_addr_ = __builtin_return_address(0);
			}

			void CheckDisposal() const
			{
				if (IsDisposed()) {
					throw std::logic_error("Attempted to use a disposed object");
				}
			}

		protected:
			virtual void PerformDispose() = 0;

		private:
			bool disposed_;
			void *dispose_addr_;
		};
	}
}
