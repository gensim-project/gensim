/*
 * File:   Translation.h
 * Author: s0457958
 *
 * Created on 21 July 2014, 13:47
 */

#ifndef TRANSLATION_H
#define	TRANSLATION_H

#include "core/thread/ThreadInstance.h"
#include "util/Lifetime.h"
#include "blockjit/ir.h"

#include <unordered_set>

namespace archsim
{
	namespace translate
	{
#define TXLN_RESULT_OK							0
#define TXLN_RESULT_MUST_INTERPRET				1
#define TXLN_RESULT_MUST_INTERPRET_AND_TRACE	5

		class Translation
		{
		public:
			bool IsRegistered() const
			{
				return is_registered;
			}

			void SetRegistered()
			{
				is_registered = true;
			}

			Translation();
			virtual ~Translation();
			virtual uint32_t Execute(archsim::core::thread::ThreadInstance *thread) = 0;
			virtual void Install(captive::shared::block_txln_fn *location) = 0;
			virtual uint32_t GetCodeSize() const = 0;

			inline void AddContainedBlock(virt_addr_t addr)
			{
				contained_blocks.insert(addr);
			}

			inline bool ContainsBlock(virt_addr_t addr) const
			{
				return contained_blocks.count(addr) > 0;
			}

			inline const std::unordered_set<virt_addr_t>& GetBlocks() const
			{
				return contained_blocks;
			}

		private:
			std::unordered_set<virt_addr_t> contained_blocks;
			bool is_registered;
		};
	}
}

#endif	/* TRANSLATION_H */
