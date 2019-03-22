/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   InterruptCheckingScheme.h
 * Author: s0457958
 *
 * Created on 05 August 2014, 14:28
 */

#ifndef INTERRUPTCHECKINGSCHEME_H
#define	INTERRUPTCHECKINGSCHEME_H

#include "define.h"
#include "abi/Address.h"
#include <map>

namespace gensim
{
	class BaseJumpInfoProvider;
}

namespace archsim
{
	namespace translate
	{
		class TranslationBlockUnit;

		namespace interrupts
		{
			class InterruptCheckingScheme
			{
			public:
				virtual ~InterruptCheckingScheme();
				virtual bool ApplyInterruptChecks(gensim::BaseJumpInfoProvider& jump_info, std::map<Address, TranslationBlockUnit *>& blocks) = 0;
			};

			class NoneInterruptCheckingScheme : public InterruptCheckingScheme
			{
			public:
				bool ApplyInterruptChecks(gensim::BaseJumpInfoProvider& jump_info, std::map<Address, TranslationBlockUnit *>& blocks);
			};

			class FullInterruptCheckingScheme : public InterruptCheckingScheme
			{
			public:
				bool ApplyInterruptChecks(gensim::BaseJumpInfoProvider& jump_info, std::map<Address, TranslationBlockUnit *>& blocks);
			};

			class BackwardsBranchCheckingScheme : public InterruptCheckingScheme
			{
			public:
				bool ApplyInterruptChecks(gensim::BaseJumpInfoProvider& jump_info, std::map<Address, TranslationBlockUnit *>& blocks);
			};
		}
	}
}

#endif	/* INTERRUPTCHECKINGSCHEME_H */

