/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ssa/SSAActionPrototype.h"

using namespace gensim::genc::ssa;

SSAActionPrototype::SSAActionPrototype(const IRSignature& signature) : _signature(signature), _return_type(signature.GetType())
{
	for (const auto& param : signature.GetParams()) {
		_parameter_types.push_back(param.GetType());
	}
}
