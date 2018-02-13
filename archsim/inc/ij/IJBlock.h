/*
 * File:   IJBlock.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 14:16
 */

#ifndef IJBLOCK_H
#define	IJBLOCK_H

#include <list>
#include <utility>

namespace gensim
{
	class BaseDecode;
}

namespace archsim
{
	namespace ij
	{
		class IJBlock
		{
		public:
			void AddInstruction(const gensim::BaseDecode& decode, addr_t offset)
			{
				instructions.push_back(std::pair<const gensim::BaseDecode *, addr_t>(&decode, offset));
			}

			const std::list<std::pair<const gensim::BaseDecode *, addr_t>>& GetInstructions() const
			{
				return instructions;
			}

		private:
			std::list<std::pair<const gensim::BaseDecode *, addr_t>> instructions;
		};
	}
}

#endif	/* IJBLOCK_H */

