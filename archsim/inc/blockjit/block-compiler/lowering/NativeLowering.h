/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NativeLowering.h
 * Author: harry
 *
 * Created on 23 May 2018, 13:34
 */

#ifndef NATIVELOWERING_H
#define NATIVELOWERING_H

#include "blockjit/ir.h"
#include "core/thread/ThreadInstance.h"
#include "util/vbitset.h"
#include "util/MemAllocator.h"

#include <string.h>

namespace captive {
	namespace arch {
		namespace jit {
			class TranslationContext;
			
			namespace lowering {
				class LoweringResult {
				public:
					LoweringResult(captive::shared::block_txln_fn fn, size_t size) : Function(fn), Size(size) {}
					
					captive::shared::block_txln_fn Function;
					size_t Size;
				};
				
				LoweringResult NativeLowering(TranslationContext &ctx, wulib::MemAllocator &allocator, archsim::core::thread::ThreadInstance *thread, const CompileResult &compile_result);
			}
		}
	}
}

#endif /* NATIVELOWERING_H */

