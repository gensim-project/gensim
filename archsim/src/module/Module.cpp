/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "module/Module.h"

using namespace archsim::module;

// FIX for gcc <7.0
namespace archsim
{
	namespace module
	{

		template<> const archsim::module::ModuleDeviceEntry *archsim::module::ModuleInfo::GetEntry<archsim::module::ModuleDeviceEntry>(const std::string& entryname) const
		{
			auto generic_entry = GetGenericEntry(entryname);
			if(generic_entry)
			{
				if(generic_entry->GetType() == archsim::module::ModuleEntry::ModuleEntry_Device) {
					return ((archsim::module::ModuleDeviceEntry*)generic_entry);
				}
			}
			return nullptr;
		}

		template<> const archsim::module::ModuleComponentEntry *archsim::module::ModuleInfo::GetEntry<archsim::module::ModuleComponentEntry>(const std::string& entryname) const
		{
			auto generic_entry = GetGenericEntry(entryname);
			if(generic_entry)
			{
				if(generic_entry->GetType() == archsim::module::ModuleEntry::ModuleEntry_Component) {
					return ((archsim::module::ModuleComponentEntry*)generic_entry);
				}
			}
			return nullptr;
		}
		
		template<> const archsim::module::ModuleExecutionEngineEntry *archsim::module::ModuleInfo::GetEntry<archsim::module::ModuleExecutionEngineEntry>(const std::string& entryname) const
		{
			auto generic_entry = GetGenericEntry(entryname);
			if(generic_entry)
			{
				if(generic_entry->GetType() == archsim::module::ModuleEntry::ModuleEntry_ExecutionEngine) {
					return ((archsim::module::ModuleExecutionEngineEntry*)generic_entry);
				}
			}
			return nullptr;
		}

		const std::string& ModuleInfo::GetName() const
		{
			return module_name_;
		}

		ModuleInfo::ModuleInfo(const std::string& module_name, const std::string& module_description) : module_name_(module_name), module_description_(module_description)
		{

		}



		ModuleEntry::ModuleEntry(const std::string& name, ModuleEntryType type) : name_(name), type_(type)
		{

		}
		
		const std::string& ModuleEntry::GetName() const
		{
			return name_;
		}

		ModuleEntry::ModuleEntryType ModuleEntry::GetType() const
		{
			return type_;
		}


		bool ModuleInfo::HasEntry(const std::string& name) const
		{
			return entries_.count(name);
		}


		bool ModuleInfo::AddEntry(ModuleEntry* entry)
		{
			if(HasEntry(entry->GetName())) {
				return false;
			}
			entries_[entry->GetName()] = entry;
			return true;
		}

		const ModuleEntry* ModuleInfo::GetGenericEntry(const std::string& entryname) const
		{
			if(!HasEntry(entryname))  {
				return nullptr;
			}

			return entries_.at(entryname);
		}


		template<> const ModuleProcessorEntry *ModuleInfo::GetEntry<>(const std::string &entryname) const
		{
			const ModuleEntry *generic_entry = GetGenericEntry(entryname);
			if(generic_entry) {
				if(generic_entry->GetType() == ModuleEntry::ModuleEntry_Processor) {
					return (const ModuleProcessorEntry*)generic_entry;
				} 
			}
			return nullptr;
		}

	}
}