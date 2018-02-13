
#include "gensim/gensim_decode_context.h"
#include "util/ComponentManager.h"

using namespace gensim;

DecodeContext::DecodeContext(gensim::Processor* cpu) : cpu_(cpu)
{

}


DecodeContext::~DecodeContext()
{

}

DefineComponentType(gensim::DecodeContext, gensim::Processor *);
DefineComponentType(gensim::DecodeTranslateContext);