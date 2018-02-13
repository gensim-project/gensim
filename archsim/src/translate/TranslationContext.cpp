#include "translate/TranslationContext.h"
#include "translate/Translation.h"
#include "translate/TranslationWorkUnit.h"
#include "gensim/gensim_processor.h"

using namespace archsim::translate;

TranslationContext::TranslationContext(TranslationManager& tmgr, TranslationWorkUnit& twu)
	:	twu(twu),
	  tmgr(tmgr),
	  mtm(twu.GetProcessor().GetMemoryModel().GetTranslationModel())
{
}

TranslationContext::~TranslationContext()
{

}

