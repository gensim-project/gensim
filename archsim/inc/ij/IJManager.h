/*
 * File:   IJManager.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 10:35
 */

#ifndef IJMANAGER_H
#define	IJMANAGER_H

#include "define.h"

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace ij
	{
		class IJManager
		{
		public:
			typedef uint32_t (*ij_block_fn)(void *state);

			IJManager();
			virtual ~IJManager();

			bool Initialise();
			void Destroy();

			ij_block_fn TranslateBlock(gensim::Processor& cpu, phys_addr_t phys_addr);
		};
	}
}

#endif	/* IJMANAGER_H */

