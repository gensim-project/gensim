/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ExecutionEngineFactory.h
 * Author: harry
 *
 * Created on 24 May 2018, 15:14
 */

#ifndef EXECUTIONENGINEFACTORY_H
#define EXECUTIONENGINEFACTORY_H

#include "core/execution/ExecutionEngine.h"
#include "module/Module.h"

#include <functional>
#include <map>

namespace archsim {
	namespace core {
		namespace execution {
			class ExecutionEngineFactory {
			public:
				using EEFactory = std::function<ExecutionEngine*(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix)>;
				
				ExecutionEngineFactory();
				ExecutionEngine *Get(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);
				
			private:
				std::multimap<uint32_t, EEFactory, std::greater<uint32_t>> factories_;
			};
		}
	}
}

#endif /* EXECUTIONENGINEFACTORY_H */

