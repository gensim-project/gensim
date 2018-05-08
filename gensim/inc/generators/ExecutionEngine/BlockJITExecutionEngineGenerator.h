/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockJITExecutionEngineGenerator.h
 * Author: harry
 *
 * Created on 26 April 2018, 13:20
 */

#ifndef BLOCKJITEXECUTIONENGINEGENERATOR_H
#define BLOCKJITEXECUTIONENGINEGENERATOR_H

#include "generators/GenerationManager.h"
#include "generators/ExecutionEngine/EEGenerator.h"

namespace gensim {
	namespace generator {
		class BlockJITExecutionEngineGenerator : public EEGenerator {
		public: 
			BlockJITExecutionEngineGenerator(GenerationManager &man);
			
			bool GenerateHeader(util::cppformatstream& str) const override;
			bool GenerateSource(util::cppformatstream& str) const override;
			const std::vector<std::string> GetSources() const override;

		private:
			mutable std::vector<std::string> sources;
		};
	}
}

#endif /* BLOCKJITEXECUTIONENGINEGENERATOR_H */

