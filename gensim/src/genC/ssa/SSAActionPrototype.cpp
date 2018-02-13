/*
 * genC/ssa/SSAContext.cpp
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#include "genC/ssa/SSAActionPrototype.h"

using namespace gensim::genc::ssa;

SSAActionPrototype::SSAActionPrototype(const IRSignature& signature) : _signature(signature), _return_type(signature.GetType())
{
	for (const auto& param : signature.GetParams()) {
		_parameter_types.push_back(param.GetType());
	}
}
