/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   int128.h
 * Author: harry
 *
 * Created on 28 June 2018, 14:46
 */

#ifndef INT128_H
#define INT128_H

namespace wutils
{

	template<bool signedness> class int128
	{
	public:
		int128() : value_(0) {}
		int128(uint64_t v) : value_(v) {}
		operator __int128()
		{
			return value_;
		}

	private:
		// TODO: this relies on gcc int128 types
		__int128 value_;
	};
}



#endif /* INT128_H */

