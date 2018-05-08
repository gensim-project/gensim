/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ModuleDescriptorGenerator.h
 * Author: harry
 *
 * Created on 26 April 2018, 15:36
 */

#ifndef MODULEDESCRIPTORGENERATOR_H
#define MODULEDESCRIPTORGENERATOR_H

#include "GenerationManager.h"

namespace gensim {
	namespace generator {
		class ModuleDescriptorGenerator : public GenerationComponent {
		public:
			ModuleDescriptorGenerator(GenerationManager &man);
			
			bool Generate() const override;

			virtual const std::vector<std::string> GetSources() const;
			std::string GetFunction() const override;



		};
	}
}

#endif /* MODULEDESCRIPTORGENERATOR_H */

