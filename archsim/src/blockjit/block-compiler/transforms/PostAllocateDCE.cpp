/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PostAllocateDCE.cpp
 *
 *  Created on: 30/6/2016
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <map>
#include <vector>
#include <set>
#include <cstdint>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

PostAllocateDCE::~PostAllocateDCE()
{

}

bool PostAllocateDCE::Apply(TranslationContext& ctx)
{
	return true;
}
