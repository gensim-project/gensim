/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/ExecutionEngineFactory.h"

using namespace archsim::core::execution;

DeclareLogContext(LogEEFactory, "EEFactory");

ExecutionEngineFactory *ExecutionEngineFactory::singleton_ = nullptr;

ExecutionEngineFactory& ExecutionEngineFactory::GetSingleton()
{
	if(singleton_ == nullptr) {
		singleton_ = new ExecutionEngineFactory();
	}
	
	return *singleton_;
}

ExecutionEngineFactory::ExecutionEngineFactory() {
}

ExecutionEngine *ExecutionEngineFactory::Get(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix) {
	LC_DEBUG1(LogEEFactory) << "Available EE Factories:";
	for(auto i : factories_) {
		LC_DEBUG1(LogEEFactory) << "  " << i.second.Name;
	}
	
	for(auto i : factories_) {
		auto result = i.second.Factory(module, cpu_prefix);
		LC_DEBUG1(LogEEFactory) << "For module " << module->GetName() << ", trying EEFactory " << i.second.Name;
		if(result) {
			LC_INFO(LogEEFactory) << "Selected EE " << i.second.Name << " for module " << module->GetName();
			return result;
		}
	}
	
	return nullptr;
}

void ExecutionEngineFactory::Register(const std::string& name, int priority, EEFactory factory)
{	
	Entry entry;
	entry.Name = name;
	entry.Factory = factory;
	entry.Priority = priority;
	factories_.insert({priority, entry});
}


ExecutionEngineFactoryRegistration::ExecutionEngineFactoryRegistration(const std::string& name, int priority, ExecutionEngineFactory::EEFactory factory)
{
	ExecutionEngineFactory::GetSingleton().Register(name, priority, factory);
}
