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
#include "gensim/ExecutionEngine.h"
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

		class ModuleEntry
		{
		public:
			enum ModuleEntryType {
				ModuleEntry_Component,
				ModuleEntry_Device,
				ModuleEntry_Processor,
				ModuleEntry_ExecutionEngine,
				ModuleEntry_ArchDescriptor,
			};

			ModuleEntry(const std::string &name, ModuleEntryType type);
			const std::string &GetName() const;
			ModuleEntryType GetType() const;

		private:
			ModuleEntryType type_;
			const std::string name_;
		};

		template<typename T> struct ModuleEntryTypeForClass {};
		template<> struct ModuleEntryTypeForClass<abi::devices::Component*> { static const ModuleEntry::ModuleEntryType entry = ModuleEntry::ModuleEntry_Component; };
		template<> struct ModuleEntryTypeForClass<abi::devices::MemoryComponent*> { static const ModuleEntry::ModuleEntryType entry = ModuleEntry::ModuleEntry_Device; };
		template<> struct ModuleEntryTypeForClass<gensim::Processor*> { static const ModuleEntry::ModuleEntryType entry = ModuleEntry::ModuleEntry_Processor; };
		template<> struct ModuleEntryTypeForClass<archsim::ExecutionEngine*> { static const ModuleEntry::ModuleEntryType entry = ModuleEntry::ModuleEntry_ExecutionEngine; };
		template<> struct ModuleEntryTypeForClass<archsim::ArchDescriptor*> { static const ModuleEntry::ModuleEntryType entry = ModuleEntry::ModuleEntry_ArchDescriptor; };
		
		template<ModuleEntry::ModuleEntryType> struct FactoryForModuleEntry {};
		template<> struct FactoryForModuleEntry<ModuleEntry::ModuleEntry_Component> { using factory_t = std::function<abi::devices::Component*(archsim::abi::EmulationModel&)>; };
		template<> struct FactoryForModuleEntry<ModuleEntry::ModuleEntry_Device> { using factory_t = std::function<abi::devices::MemoryComponent*(archsim::abi::EmulationModel& model, archsim::Address base_address)>; };
		template<> struct FactoryForModuleEntry<ModuleEntry::ModuleEntry_Processor> { using factory_t = std::function<gensim::Processor*(const std::string &name, int _core_id, archsim::util::PubSubContext* pubsub)>; };
		template<> struct FactoryForModuleEntry<ModuleEntry::ModuleEntry_ExecutionEngine> { using factory_t = std::function<archsim::ExecutionEngine*()>; };
		template<> struct FactoryForModuleEntry<ModuleEntry::ModuleEntry_ArchDescriptor> { using factory_t = std::function<archsim::ArchDescriptor*()>; };
		
		template<typename T> class TypedModuleEntry : public ModuleEntry {
		public:
			static constexpr ModuleEntry::ModuleEntryType kEntry = ModuleEntryTypeForClass<T>::entry;
			using factory_t = typename FactoryForModuleEntry<kEntry>::factory_t;
			
			TypedModuleEntry(const std::string &name, factory_t factory) : ModuleEntry(name, kEntry), Get(factory) { }
			const factory_t Get;
		};
		
		using ModuleComponentEntry = TypedModuleEntry<abi::devices::Component*>;
		using ModuleDeviceEntry = TypedModuleEntry<abi::devices::MemoryComponent*>;
		using ModuleProcessorEntry = TypedModuleEntry<gensim::Processor*>;
		using ModuleExecutionEngineEntry = TypedModuleEntry<archsim::ExecutionEngine*>;
		using ModuleArchDescriptorEntry = TypedModuleEntry<archsim::ArchDescriptor*>;
		
		class ModuleInfo
		{
		public:
			ModuleInfo(const std::string &module_name, const std::string &module_description);
			const std::string &GetName() const;

			void AddDependency(const std::string &dependency_name);

			bool AddEntry(ModuleEntry *entry);
			bool HasEntry(const std::string &name) const;

			const ModuleEntry *GetGenericEntry(const std::string &entryname) const;

			template<typename T> const T *GetEntry(const std::string &entryname) const {
				auto generic_entry = GetGenericEntry(entryname);
				if(generic_entry != nullptr) {
					if(generic_entry->GetType() == T::kEntry) {
						return (T*)generic_entry;
					}
				}
				
				return nullptr;
			}

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
#define ARCHSIM_EEFACTORY(x) []() { static_assert(std::is_base_of<archsim::ExecutionEngine,x>::value, "Component must be a EE"); return new x(); }
#define ARCHSIM_ARCHDESCRIPTORFACTORY(x) []() { static_assert(std::is_base_of<archsim::ArchDescriptor,x>::value, "Component must be a AD"); return new x(); }

#endif /* MODULE_H */

