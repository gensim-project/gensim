/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/* 
 * File:   IRPrinter.h
 * Author: harry
 *
 * Created on 16 May 2018, 15:00
 */

#ifndef IRPRINTER_H
#define IRPRINTER_H

#include <sstream>
#include "blockjit/translation-context.h"

namespace archsim {
	namespace blockjit {
		class IRPrinter {
		public:
			void DumpIR(std::ostream &str, const captive::arch::jit::TranslationContext &ctx);
		};
	}
}

#endif /* IRPRINTER_H */

