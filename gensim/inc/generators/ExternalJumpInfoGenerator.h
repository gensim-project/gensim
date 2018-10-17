/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   ExternalDecodeGenerator.h
 * Author: harry
 *
 * Created on 09 July 2018, 15:38
 */

#ifndef EXTERNALJUMPINFOGENERATOR_H
#define EXTERNALJUMPINFOGENERATOR_H

#include "arch/ArchDescription.h"
#include "generators/GenerationManager.h"

namespace gensim
{
	namespace generator
	{

		class ExternalJumpInfoGenerator : public GenerationComponent
		{
		public:
			ExternalJumpInfoGenerator(GenerationManager &man) : GenerationComponent(man, "external_jumpinfo") {}
			virtual ~ExternalJumpInfoGenerator() {}

			virtual bool Generate() const;

			virtual std::string GetFunction() const
			{
				return GenerationManager::FnJumpInfo;
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

