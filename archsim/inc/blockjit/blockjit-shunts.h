/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * blockjit-shunts.h
 *
 *  Created on: 21 Sep 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCKJIT_SHUNTS_H_
#define INC_BLOCKJIT_BLOCKJIT_SHUNTS_H_

#include <cstdint>

extern "C" {

	struct fallback_return_t {
		uint32_t ignore;
		void *data;
	} ;

	extern uint32_t blkFnFallbackSlot;
	fallback_return_t blkFnRead8Fallback(void *processor, uint32_t address);
	fallback_return_t blkFnRead16Fallback(void *processor, uint32_t address);
	fallback_return_t blkFnRead32Fallback(void *processor, uint32_t address);

	fallback_return_t blkFnRead8UserFallback(void *processor, uint32_t address);
	fallback_return_t blkFnRead16UserFallback(void *processor, uint32_t address);
	fallback_return_t blkFnRead32UserFallback(void *processor, uint32_t address);

	fallback_return_t blkFnWrite8Fallback(void *processor, uint32_t address, uint32_t value);
	fallback_return_t blkFnWrite16Fallback(void *processor, uint32_t address, uint32_t value);
	fallback_return_t blkFnWrite32Fallback(void *processor, uint32_t address, uint32_t value);
}

#endif /* INC_BLOCKJIT_BLOCKJIT_SHUNTS_H_ */
