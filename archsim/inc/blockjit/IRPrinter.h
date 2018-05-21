/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

