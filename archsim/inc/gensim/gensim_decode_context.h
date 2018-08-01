/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   gensim_decode_context.h
 * Author: harry
 *
 * Created on 02 May 2016, 11:05
 */

#ifndef GENSIM_DECODE_CONTEXT_H
#define GENSIM_DECODE_CONTEXT_H

#include "abi/Address.h"
#include "util/Cache.h"
#include "util/PubSubSync.h"

namespace captive
{
	namespace shared
	{
		class IRBuilder;
	}
	namespace arch
	{
		namespace jit
		{
			class TranslationContext;
		}
	}
}

namespace archsim
{
	class MemoryInterface;
	namespace core
	{
		namespace thread
		{
			class ThreadInstance;
		}
	}
}

namespace gensim
{
	class BaseDecode;
	class Processor;

	class DecodeContext
	{
	public:
		DecodeContext();
		virtual ~DecodeContext();
		virtual uint32_t DecodeSync(archsim::MemoryInterface &mem_interface, archsim::Address address, uint32_t mode, BaseDecode *&target) = 0;

		virtual void Reset(archsim::core::thread::ThreadInstance *thread);
		virtual void WriteBackState(archsim::core::thread::ThreadInstance *thread);
	};

	class CachedDecodeContext : public DecodeContext
	{
	public:
		CachedDecodeContext(archsim::util::PubSubContext &pubsub, DecodeContext *underlying_ctx, std::function<gensim::BaseDecode*()> new_decode_fn);
		uint32_t DecodeSync(archsim::MemoryInterface& mem_interface, archsim::Address address, uint32_t mode, BaseDecode *&target) override;

		void Reset(archsim::core::thread::ThreadInstance* thread) override;
		void WriteBackState(archsim::core::thread::ThreadInstance* thread) override;

		void Flush();

	private:
		archsim::util::Cache<archsim::Address, gensim::BaseDecode*> decode_cache_;
		std::function<gensim::BaseDecode*()> new_decode_fn_;
		DecodeContext *underlying_ctx_;
		archsim::util::PubSubContext &pubsub_;
	};

	// This class is used to emit operations which should happen unconditionally
	// before an instruction executes.
	class DecodeTranslateContext
	{
	public:
		virtual void Translate(archsim::core::thread::ThreadInstance *cpu, const gensim::BaseDecode &insn, DecodeContext &decode, captive::shared::IRBuilder &builder) = 0;
	};
}

#endif /* GENSIM_DECODE_CONTEXT_H */

