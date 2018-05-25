#include "translate/TranslationContext.h"
#include "translate/Translation.h"
#include "translate/TranslationWorkUnit.h"

using namespace archsim::translate;

TranslationContext::TranslationContext(TranslationManager& tmgr, TranslationWorkUnit& twu)
	:	twu(twu),
	  tmgr(tmgr)
{
	
}

TranslationContext::~TranslationContext()
{

}

