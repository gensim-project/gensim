
#include "gensim/gensim_decode_context.h"
#include "util/ComponentManager.h"

using namespace gensim;

DecodeContext::DecodeContext(archsim::core::thread::ThreadInstance* cpu) : cpu_(cpu)
{

}


DecodeContext::~DecodeContext()
{

}

DefineComponentType(gensim::DecodeContext, archsim::core::thread::ThreadInstance *);
DefineComponentType(gensim::DecodeTranslateContext);