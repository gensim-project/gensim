/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TranslationContext.h
 * Author: s0457958
 *
 * Created on 16 July 2014, 15:57
 */

#ifndef TRANSLATIONCONTEXT_H
#define	TRANSLATIONCONTEXT_H

#include <stdint.h>

namespace gensim
{
	class BaseDecode;
	class BaseTranslate;
}

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class MemoryTranslationModel;
		}
	}

	namespace translate
	{
		class Translation;
		class TranslationManager;
		class TranslationWorkUnit;
		class TranslationBlockUnit;
		class TranslationInstructionUnit;
		struct TranslationTimers;

		class TranslationContext
		{
		public:
			TranslationContext(const TranslationContext&) = delete;
			TranslationContext(TranslationManager& tmgr, TranslationWorkUnit& twu);
			virtual ~TranslationContext();

			virtual bool Translate(Translation*& translation, TranslationTimers& timers) = 0;

			TranslationWorkUnit& twu;

		private:
			TranslationManager& tmgr;
		};

		template<class TParentContext>
		class RegionTranslationContext
		{
		public:
			RegionTranslationContext(const RegionTranslationContext &) = delete;
			RegionTranslationContext(TParentContext& parent);
			TParentContext& txln;
		};

		template<class TParentContext>
		class BlockTranslationContext
		{
		public:
			BlockTranslationContext(const BlockTranslationContext&) = delete;
			BlockTranslationContext(TParentContext& parent, TranslationBlockUnit& tbu);

			virtual ~BlockTranslationContext();

			virtual bool Translate() = 0;

			TParentContext& region;
			const TranslationBlockUnit& tbu;
		};

		template<class TParentContext>
		class InstructionTranslationContext
		{
		public:
			InstructionTranslationContext(TParentContext& parent, TranslationInstructionUnit& tiu);
			virtual ~InstructionTranslationContext();

			virtual bool Translate() = 0;

			TParentContext& block;
			const TranslationInstructionUnit& tiu;
		};
	}
}

#endif	/* TRANSLATIONCONTEXT_H */

