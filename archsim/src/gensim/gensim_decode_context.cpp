
#include "gensim/gensim_decode_context.h"
#include "util/ComponentManager.h"

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


DefineComponentType(gensim::DecodeContext);
DefineComponentType(gensim::DecodeTranslateContext);