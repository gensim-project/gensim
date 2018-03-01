/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   ModuleManager.h
 * Author: harry
 *
 * Created on 29 November 2017, 15:31
 */

#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include "Module.h"

#include <map>
#include <string>

namespace archsim
{
	namespace module
	{
		class ModuleManager
		{
		public:
			bool LoadModule(const std::string &module_filename);
			bool LoadModuleDirectory(const std::string &module_directory);
			bool LoadStandardModuleDirectory();

			bool AddModule(ModuleInfo *mod_info);

			bool HasModule(const std::string &module_name) const;

			const ModuleInfo *GetModule(const std::string &module_name) const;

			template<typename T> const T* GetModuleEntry(const std::string &fully_qualified_name) const
			{
				const ModuleInfo *module = GetModuleByPrefix(fully_qualified_name);
				std::string entry_name = fully_qualified_name.substr(module->GetName().size() + 1);

				return module->GetEntry<T>(entry_name);
			}

		private:
			const ModuleInfo *GetModuleByPrefix(const char *str) const;
			const ModuleInfo *GetModuleByPrefix(const std::string &fully_qualified_name) const;

			std::map<std::string, ModuleInfo *> loaded_modules_;

		};
	}
}

#endif /* MODULEMANAGER_H */

