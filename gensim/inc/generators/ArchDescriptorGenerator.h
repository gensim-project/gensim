/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ArchDescriptorGenerator.h
 * Author: harry
 *
 * Created on 12 April 2018, 09:50
 */

#ifndef ARCHDESCRIPTORGENERATOR_H
#define ARCHDESCRIPTORGENERATOR_H

#include "GenerationManager.h"

namespace gensim
{
	namespace generator
	{

		class ArchDescriptorGenerator : public GenerationComponent {
		public:
			ArchDescriptorGenerator(GenerationManager &manager);
			
			bool Generate() const override;
			std::string GetFunction() const override;
			const std::vector<std::string> GetSources() const override;
			
		private:
			bool GenerateHeader() const;
			bool GenerateSource() const;
		};
		
	}
}

#endif /* ARCHDESCRIPTORGENERATOR_H */

