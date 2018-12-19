/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LexicographicallyOrderBlocksPass.h
 * Author: harry
 *
 * Created on 19 December 2018, 16:40
 */

#ifndef LEXICOGRAPHICALLYORDERBLOCKSPASS_H
#define LEXICOGRAPHICALLYORDERBLOCKSPASS_H

#include <llvm/Pass.h>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{
			class LexicographicallyOrderBlocksPass : public llvm::FunctionPass
			{
			public:
				LexicographicallyOrderBlocksPass();
				virtual bool runOnFunction(llvm::Function& F);

			private:
				static char pid;
			};
		}
	}
}

#endif /* LEXICOGRAPHICALLYORDERBLOCKSPASS_H */

