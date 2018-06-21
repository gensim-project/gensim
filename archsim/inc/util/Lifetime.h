/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Lifetime.h
 * Author: s0457958
 *
 * Created on 07 August 2014, 11:18
 */

#ifndef LIFETIME_H
#define	LIFETIME_H

#include "define.h"
#include <atomic>
#include <cassert>
#include <mutex>
#include <list>

namespace archsim
{
	namespace util
	{
		class ReferenceCounted
		{
		public:
			ReferenceCounted()
			{
				ref_count = 1;
			}
			virtual ~ReferenceCounted() { }

			inline void Acquire()
			{
				if (ref_count++ == 0) assert(false);
			}
			inline void Release()
			{
				assert(ref_count);
				if (--ref_count == 0) {
					delete this;
				}
			}

			inline uint32_t References() const
			{
				return ref_count;
			}

		private:
			std::atomic<uint32_t> ref_count;
		};
	}
}

#endif	/* LIFETIME_H */

