/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "gensim/gensim_decode_context.h"
#include "util/ComponentManager.h"
#include "gensim/gensim_decode.h"

using namespace gensim;

DecodeContext::DecodeContext()
{

}


DecodeContext::~DecodeContext()
{

}

void DecodeContext::Reset(archsim::core::thread::ThreadInstance* thread)
{

}

void DecodeContext::WriteBackState(archsim::core::thread::ThreadInstance* thread)
{

}

void FlushCallback(PubSubType::PubSubType, void *ctx, const void *data)
{
	CachedDecodeContext *d = (CachedDecodeContext*)ctx;
	d->Flush();
}

void purge_decode(gensim::BaseDecode*& d)
{
	if(d != nullptr) {
		d->Release();
		d = nullptr;
	}
}

CachedDecodeContext::CachedDecodeContext(archsim::util::PubSubContext& pubsub, DecodeContext* underlying_ctx, std::function<gensim::BaseDecode*() > new_decode_fn) : underlying_ctx_(underlying_ctx), new_decode_fn_(new_decode_fn), pubsub_(pubsub), decode_cache_(purge_decode)
{
	pubsub_.Subscribe(PubSubType::FlushTranslations, FlushCallback, this);
	pubsub_.Subscribe(PubSubType::FlushAllTranslations, FlushCallback, this);
}


uint32_t CachedDecodeContext::DecodeSync(archsim::MemoryInterface& mem_interface, archsim::Address address, uint32_t mode, BaseDecode*& target)
{
	gensim::BaseDecode **cache_ptr;
	auto result = decode_cache_.try_cache_fetch(address, cache_ptr);

	switch(result) {
		case archsim::util::CACHE_MISS: {
			if(*cache_ptr != nullptr) {
				(*cache_ptr)->Release();
			}

			auto result = underlying_ctx_->DecodeSync(mem_interface, address, mode, *cache_ptr);
			(*cache_ptr)->Acquire();
			target = *cache_ptr;
			return result;
		}
		case archsim::util::CACHE_HIT_WAY_0:
		case archsim::util::CACHE_HIT_WAY_1:
			target = *cache_ptr;
			target->Acquire();
			return 0;

		default:
			UNEXPECTED;
	}


}

void CachedDecodeContext::Reset(archsim::core::thread::ThreadInstance* thread)
{
	underlying_ctx_->Reset(thread);
}

void CachedDecodeContext::WriteBackState(archsim::core::thread::ThreadInstance* thread)
{
	underlying_ctx_->WriteBackState(thread);
}

void CachedDecodeContext::Flush()
{
	decode_cache_.purge();
}


DefineComponentType(gensim::DecodeContext);
DefineComponentType(gensim::DecodeTranslateContext);
