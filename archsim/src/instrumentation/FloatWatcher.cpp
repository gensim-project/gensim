/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * FloatWatcher.cpp
 *
 *  Created on: 24 Jun 2015
 *      Author: harry
 */
#include "instrumentation/FloatWatcher.h"

#include <cxxabi.h>

void float_callback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	FloatWatcher *watcher = (FloatWatcher*)ctx;
	struct FloatStruct *d = (struct FloatStruct *)data;

	watcher->RegisterValue(d->pc, d->value);
}

FloatWatcher::FloatWatcher(System &sys) : system(sys)
{
	sys.GetPubSub().Subscribe(PubSubType::FloatResult, float_callback, this);
}

FloatWatcher::~FloatWatcher()
{

}

std::string demangle(std::string input);

void FloatWatcher::Dump()
{
	const archsim::abi::BinarySymbol *symbol = NULL;
	std::cout << "Float values:" << std::endl;
	for(auto i : min_values) {
		if(!symbol || !symbol->Contains(i.first))
			system.GetEmulationModel().LookupSymbol(i.first, false, symbol);

		std::string symbolname;
		if(symbol) {
			symbolname = demangle(symbol->Name);
		}
		std::cout << symbolname << " " << std::hex << i.first << " " << std::dec << i.second << " " << max_values[i.first] << (requires_float.count(i.first) ? "" : " (I)") << std::endl;
	}
}

void FloatWatcher::RegisterValue(uint32_t pc, float value)
{
	if(!min_values.count(pc)) min_values[pc] = value;
	else if(min_values[pc] > value) min_values[pc] = value;

	if(!max_values.count(pc)) max_values[pc] = value;
	else if(max_values[pc] < value) max_values[pc] = value;

	if((int)value != value) requires_float.insert(pc);

//	assert(max_values[pc] >= min_values[pc]);
}
