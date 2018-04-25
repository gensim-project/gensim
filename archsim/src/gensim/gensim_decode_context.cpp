
#include "gensim/gensim_decode_context.h"
#include "util/ComponentManager.h"

using namespace gensim;

DecodeContext::DecodeContext(archsim::ThreadInstance* cpu) : cpu_(cpu)
{

}


DecodeContext::~DecodeContext()
{

}

DefineComponentType(gensim::DecodeContext, archsim::ThreadInstance *);
DefineComponentType(gensim::DecodeTranslateContext);