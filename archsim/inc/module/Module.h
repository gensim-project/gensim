/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Module.h
 * Author: harry
 *
 * Created on 29 November 2017, 13:18
 */

#ifndef MODULE_H
#define MODULE_H

#include "abi/Address.h"
#include "abi/devices/Component.h"
#include "util/PubSubSync.h"
#include <functional>
#include <string>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace module
	{

		class ModuleManager;

		typedef std::function<abi::devices::MemoryComponent *(archsim::abi::EmulationModel &, archsim::Address)> memory_component_factory_t;
		typedef std::function<abi::devices::Component *(archsim::abi::EmulationModel &)> component_factory_t;
		typedef std::function<gensim::Processor *(const std::string &, int, archsim::util::PubSubContext*)> processor_factory_t;

		class ModuleEntry
		{
		public:
			enum ModuleEntryType {
				ModuleEntry_Component,
				ModuleEntry_Device,
				ModuleEntry_Processor
			};

			ModuleEntry(const std::string &name, ModuleEntryType type);
			const std::string &GetName() const;
			ModuleEntryType GetType() const;

		private:
			ModuleEntryType type_;
			const std::string name_;
		};

		class ModuleComponentEntry : public ModuleEntry
		{
		public:
			ModuleComponentEntry(const std::string &name, component_factory_t factory);
			abi::devices::Component *Get(archsim::abi::EmulationModel &) const;

		private:
			component_factory_t factory_;
		};

		class ModuleDeviceEntry : public ModuleEntry
		{
		public:
			ModuleDeviceEntry(const std::string &name, memory_component_factory_t factory);
			abi::devices::MemoryComponent *Get(archsim::abi::EmulationModel &, archsim::Address) const;

		private:
			memory_component_factory_t factory_;
		};

		class ModuleProcessorEntry : public ModuleEntry
		{
		public:
			ModuleProcessorEntry(const std::string &name, processor_factory_t factory);
			gensim::Processor *Get(const std::string &, int, archsim::util::PubSubContext*) const;

		private:
			processor_factory_t factory_;
		};

		class ModuleInfo
		{
		public:
			ModuleInfo(const std::string &module_name, const std::string &module_description);
			const std::string &GetName() const;

			void AddDependency(const std::string &dependency_name);

			bool AddEntry(ModuleEntry *entry);
			bool HasEntry(const std::string &name) const;

			const ModuleEntry *GetGenericEntry(const std::string &entryname) const;

			template<typename T> const T *GetEntry(const std::string &entryname) const;

		private:
			const std::string module_name_;
			const std::string module_description_;

			std::map<std::string, ModuleEntry *> entries_;
		};

		typedef ModuleInfo *(*module_loader_t)(ModuleManager*);

	}
}

#define ARCHSIM_MODULE() extern "C" archsim::module::ModuleInfo *archsim_module_(archsim::module::ModuleManager*)
#define ARCHSIM_MODULE_END() extern "C"  void archsim_module_end_(void)

#define ARCHSIM_DEVICEFACTORY(x) [](archsim::abi::EmulationModel& model, archsim::Address base_address) { static_assert(std::is_base_of<archsim::abi::devices::MemoryComponent,x>::value, "Component must be a MemoryComponent"); return new x(model, base_address); }
#define ARCHSIM_COMPONENTFACTORY(x) [](archsim::abi::EmulationModel& model) { static_assert(std::is_base_of<archsim::abi::devices::Component,x>::value, "Component must be a Component"); return new x(model); }
#define ARCHSIM_PROCESSORFACTORY(x) [](const std::string &name, int _core_id, archsim::util::PubSubContext* pubsub) { static_assert(std::is_base_of<gensim::Processor,x>::value, "Component must be a Processor"); return new x(name, _core_id, pubsub); }

#endif /* MODULE_H */

