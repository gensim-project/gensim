/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   X86JumpInfo.h
 * Author: harry
 *
 * Created on 13 September 2018, 14:10
 */

#ifndef X86JUMPINFO_H
#define X86JUMPINFO_H

#include "gensim/gensim_translate.h"

namespace archsim
{
	namespace arch
	{
		namespace x86
		{
			class X86JumpInfoProvider : public gensim::BaseJumpInfoProvider
			{
			public:
				virtual ~X86JumpInfoProvider();
				void GetJumpInfo(const gensim::BaseDecode* instr, archsim::Address pc, gensim::JumpInfo& info) override;

			};
		}
	}
}

#endif /* X86JUMPINFO_H */

