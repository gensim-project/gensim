/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EEGenerator.h
 * Author: harry
 *
 * Created on 11 April 2018, 13:42
 */

#ifndef EEGENERATOR_H
#define EEGENERATOR_H

#include "generators/GenerationManager.h"

namespace gensim {
	namespace generator {
		
		class EEGenerator : public GenerationComponent  {
		public:
			EEGenerator(GenerationManager &manager, const std::string &name) : GenerationComponent(manager, "ExecutionEngine"), name_(name) {}
			
			bool Generate() const override;
			std::string GetFunction() const override;
			const virtual std::vector<std::string> GetSources() const override;

			const std::string &GetName() const { return name_; }
			
			virtual bool GenerateHeader(util::cppformatstream &str) const;
			virtual bool GenerateSource(util::cppformatstream &str) const;

			~EEGenerator();
		private:
			std::string name_;
		};
		
	}
}

#endif /* EEGENERATOR_H */

