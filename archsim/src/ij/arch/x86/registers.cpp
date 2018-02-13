/*
 * registers.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: harry
 */

#include <cassert>
#include <cstdint>

#define X86_MACROS_INTERNAL
#include "ij/arch/x86/registers.h"



using namespace x86::reg;

RegInfo::RegInfo()
{
	//Fill in rn
	for(int i = 0; i < RegCOUNT; ++i) {
		rn[i] = ::RN((RegId)i,1);
		rs[i] = ::RS((RegId)i,1);
	}
}

const RegInfo x86::reg::info;
