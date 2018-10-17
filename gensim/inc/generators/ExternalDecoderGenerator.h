/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   ExternalDecodeGenerator.h
 * Author: harry
 *
 * Created on 09 July 2018, 15:38
 */

#ifndef EXTERNALDECODERGENERATOR_H
#define EXTERNALDECODERGENERATOR_H

#include "arch/ArchDescription.h"
#include "generators/GenerationManager.h"

namespace gensim
{
	namespace generator
	{

		class ExternalDecoder : public GenerationComponent
		{
		public:
			ExternalDecoder(GenerationManager &man) : GenerationComponent(man, "external_decoder") {}
			virtual ~ExternalDecoder() {}

			virtual bool Generate() const;

			virtual std::string GetFunction() const
			{
				return GenerationManager::FnDecode;
			}

			virtual const std::vector<std::string> GetSources() const
			{
				return {};
			}

			virtual void Reset() {}

			virtual void Setup(GenerationSetupManager& Setup) {}

		};

	}
}

#endif /* EXTERNALDECODERGENERATOR_H */

