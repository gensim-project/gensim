/*
 * File:   IJTranslationContext.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 12:57
 */

#ifndef IJTRANSLATIONCONTEXT_H
#define	IJTRANSLATIONCONTEXT_H

#include "ij/IJManager.h"
#include "ij/IJIR.h"

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace ij
	{
		class IJBlock;

		class IJTranslationContext
		{
		public:
			IJTranslationContext(IJManager& mgr, IJBlock& block, gensim::Processor& cpu);
			IJManager::ij_block_fn Translate();

			IJIR ir;

		private:
			bool GenerateIJIR();
			bool OptimiseIJIR();
			bool CompileIJIR(IJManager::ij_block_fn& fnp);

			IJManager& mgr;
			IJBlock& block;
			gensim::Processor& cpu;
		};
	}
}

#endif	/* IJTRANSLATIONCONTEXT_H */

