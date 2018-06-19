/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


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
				
				static ExecutionEngineFactory &GetSingleton();
				
				ExecutionEngine *Get(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);
				void Register(const std::string &name, int priority, EEFactory factory);
				
			private:
				ExecutionEngineFactory();
				static ExecutionEngineFactory *singleton_;
				
				class Entry {
				public:
					EEFactory Factory;
					uint32_t Priority;
					std::string Name;
				};
				
				std::multimap<uint32_t, Entry, std::greater<uint32_t>> factories_;
			};
			
			class ExecutionEngineFactoryRegistration {
			public:
				ExecutionEngineFactoryRegistration(const std::string &name, int priority, ExecutionEngineFactory::EEFactory factory);
			};
		}
	}
}

#endif /* EXECUTIONENGINEFACTORY_H */

