/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Block.h
 * Author: s0457958
 *
 * Created on 16 July 2014, 15:06
 */

#ifndef BLOCK_H
#define	BLOCK_H

#include "define.h"
#include "translate/profile/Region.h"

#include <unordered_set>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace translate
	{
		class Translation;

		namespace profile
		{
			class Block
			{
			public:
				enum BlockStatus {
					NotTranslated,
					InTranslation,
					HasTranslation,
				};

				Block(Region& parent, addr_t offset, uint8_t isa_mode);

				inline Region& GetParent() const
				{
					return parent;
				}

				inline addr_t GetOffset() const
				{
					return offset;
				}

				inline uint8_t GetISAMode() const
				{
					return isa_mode;
				}

				inline void AddSuccessor(Block& successor)
				{
					for(auto block : successors) if(block == &successor) return;
					successors.push_back(&successor);
				}

				inline const std::vector<Block *>& GetSuccessors() const
				{
					return successors;
				}

				inline BlockStatus GetStatus() const
				{
					return status;
				}

				inline void SetStatus(BlockStatus new_status)
				{
					status = new_status;
				}

				inline bool IsRootBlock() const
				{
					return root;
				}

				inline void SetRoot()
				{
					root = true;
				}

			private:
				std::vector<Block *> successors;
				Region& parent;

				addr_t offset;

				BlockStatus status;
				uint8_t isa_mode;
				bool root;
			};
		}
	}
}

#endif	/* BLOCK_H */

