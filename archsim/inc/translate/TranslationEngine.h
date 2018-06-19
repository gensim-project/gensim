/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TranslationEngine.h
 * Author: s0457958
 *
 * Created on 07 August 2014, 15:50
 */

#ifndef TRANSLATIONENGINE_H
#define	TRANSLATIONENGINE_H

namespace archsim
{
	namespace translate
	{
		class TranslationManager;

		class TranslationEngine
		{
		public:
			TranslationEngine();
			virtual ~TranslationEngine();

			virtual bool Initialise() = 0;
			virtual void Destroy() = 0;

			virtual bool Translate(TranslationManager& mgr) = 0;
		};
	}
}

#endif	/* TRANSLATIONENGINE_H */

