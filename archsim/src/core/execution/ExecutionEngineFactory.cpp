/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

ExecutionEngineFactory::ExecutionEngineFactory()
{
}

ExecutionEngine *ExecutionEngineFactory::Get(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix)
{
	LC_DEBUG1(LogEEFactory) << "Available EE Factories:";
	for(auto i : factories_) {
		LC_DEBUG1(LogEEFactory) << "  " << i.second.Name;
	}

	if(archsim::options::Mode.IsSpecified()) {
		// try and find the specified mode
		for(auto i : factories_) {
			if(i.second.Name == archsim::options::Mode.GetValue()) {
				auto result = i.second.Factory(module, cpu_prefix);
				if(result) {
					return result;
				} else {
					LC_ERROR(LogEEFactory) << "Tried to use specified mode " << archsim::options::Mode.GetValue() << ", but it returned an error";
					return nullptr;
				}
			}
		}

		LC_ERROR(LogEEFactory) << "Tried to use specified mode " << archsim::options::Mode.GetValue() << ", but it could not be found";
		return nullptr;
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
