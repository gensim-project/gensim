/*
 * CallExtractor.h
 *
 *  Created on: 26 Jun 2015
 *      Author: harry
 */

#ifndef INC_INSTRUMENTATION_CALLEXTRACTOR_H_
#define INC_INSTRUMENTATION_CALLEXTRACTOR_H_

#include "system.h"
#include "abi/EmulationModel.h"
#include "util/PubSubSync.h"

#include <sstream>

class CallExtractor
{
public:
	CallExtractor(System &sys);

	void Call(const archsim::abi::BinarySymbol *callee);
	void Return();

	void InstructionReference(uint32_t insn_type);

	void Dump(std::ostream &str);

private:
	System &system;

	std::list<const archsim::abi::BinarySymbol *> call_stack;
	std::map<const archsim::abi::BinarySymbol *, std::map<uint32_t, uint64_t>> stats;
};


#endif /* INC_INSTRUMENTATION_CALLEXTRACTOR_H_ */
