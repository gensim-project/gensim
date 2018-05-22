/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "module/ModuleManager.h"
#include "util/LogContext.h"
#include "util/string_util.h"

#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

using namespace archsim::module;

DeclareLogContext(LogModule, "Module");

bool ModuleManager::LoadModule(const std::string& module_filename)
{
	void *lib = dlopen(module_filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
	if(lib == nullptr) {
		LC_ERROR(LogModule) << "Failed to load library '" << module_filename << "': " << dlerror();
		return false;
	}

	module_loader_t module_init = (module_loader_t)dlsym(lib, "archsim_module_");
	if(module_init == nullptr) {
		LC_ERROR(LogModule) << "Failed to initialise library '" << module_filename << "': " << dlerror();
		return false;
	}

	return AddModule(module_init(this));
}

bool ModuleManager::LoadModuleDirectory(const std::string& module_directory)
{
	DIR *dir = opendir(module_directory.c_str());
	struct dirent *ent;
	bool success = true;

	if(dir) {
		while((ent = readdir(dir)) != nullptr) {
			if(strcmp(".", ent->d_name) == 0) {
				continue;
			}
			if(strcmp("..", ent->d_name) == 0) {
				continue;
			}
			success &= LoadModule(module_directory + "/" + std::string(ent->d_name));
		}
	}
	
	closedir(dir);
	return success;
}

bool ModuleManager::LoadStandardModuleDirectory()
{
	return LoadModuleDirectory(archsim::options::ModuleDirectory);
}

bool ModuleManager::AddModule(ModuleInfo* mod_info)
{
	if(HasModule(mod_info->GetName())) {
		return false;
	}

	loaded_modules_[mod_info->GetName()] = mod_info;

	return true;
}

bool ModuleManager::HasModule(const std::string& module_name) const
{
	return loaded_modules_.count(module_name) != 0;
}

const ModuleInfo* ModuleManager::GetModule(const std::string& module_name) const
{
	if(loaded_modules_.count(module_name) == 0) {
		throw std::logic_error("There is no loaded module called " + module_name);
	}
	return loaded_modules_.at(module_name);
}

const ModuleInfo* ModuleManager::GetModuleByPrefix(const char *str) const
{
	return GetModuleByPrefix(std::string(str));
}

const ModuleInfo* ModuleManager::GetModuleByPrefix(const std::string& fully_qualified_name) const
{
	// split the fqn
	auto atoms = util::split_string(fully_qualified_name, '.');

	// now try to find a module, atom by atom
	for(auto it = atoms.begin(); it != atoms.end(); ++it) {
		auto module_name = util::join_string(atoms.begin(), it, '.');

		if(HasModule(module_name)) {
			return GetModule(module_name);
		}
	}

	throw std::logic_error("Could not find a module entry matching " + fully_qualified_name);
}
