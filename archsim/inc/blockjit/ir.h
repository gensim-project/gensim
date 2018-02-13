/*
 * File:   shared-jit.h
 * Author: spink
 *
 * Created on 27 March 2015, 17:49
 */

#ifndef SHARED_JIT_H
#define	SHARED_JIT_H

#define __packed
/*
#ifndef __packed
#ifndef packed
#define __packed __attribute__((packed))
#else
#define __packed packed
#endif
#endif
*/

#include <cassert>
#include <cstdint>

#define NOP_BLOCK 0x7fffffff

namespace captive
{
	namespace shared
	{
		typedef uint32_t IRBlockId;
		typedef uint32_t IRRegId;

#define INVALID_BLOCK_ID ((IRBlockId)-1)

		struct IRInstruction;

		typedef uint32_t (*block_txln_fn)();


	}
}

#endif	/* SHARED_JIT_H */

