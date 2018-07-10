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
		int128(uint64_t v) : low_(v), high_(0) {}
		operator uint64_t()
		{
			return low_;
		}

	private:
		uint64_t low_, high_;
	};
}

#endif /* INT128_H */

