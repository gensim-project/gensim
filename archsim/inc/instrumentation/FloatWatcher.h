/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * FloatWatcher.h
 *
 *  Created on: 24 Jun 2015
 *      Author: harry
 */

#ifndef INC_FLOATWATCHER_H_
#define INC_FLOATWATCHER_H_

#include "system.h"
#include "gensim/gensim_processor.h"
#include "abi/EmulationModel.h"
#include "concurrent/ThreadsafeQueue.h"

#include <unordered_map>

struct FloatStruct {
	uint32_t pc;
	float value;
};

class FloatWatcher
{
public:
	FloatWatcher(System &sys);
	~FloatWatcher();

	void Dump();

	void RegisterValue(uint32_t pc, float value);

private:
	System& system;

	std::map<uint32_t, float> min_values;
	std::unordered_map<uint32_t, float> max_values;
	std::unordered_set<uint32_t> requires_float;
};


#endif /* INC_FLOATWATCHER_H_ */
