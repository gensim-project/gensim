#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

#include "util/system/SharedLibrary.h"
#include "util/ModuleManager.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"

using namespace archsim::util;

DeclareLogContext(LogModules, "Module");

std::list<Module *> ModuleManager::LoadedModules;

Module::Module(std::string filename_) : filename(filename_) { }

bool Module::Load()
{
	return shlib.Open(filename);
}

void Module::Unload()
{
	shlib.Close();
}

bool ModuleManager::LoadModules()
{
	struct dirent *dirent;
	std::string modules_dir;

	// If the modules directory was specified, use that.  Otherwise,
	// calculate a relative path from the binary and not the CWD.
	if (archsim::options::ModuleDirectory.IsSpecified()) {
		modules_dir = (std::string)archsim::options::ModuleDirectory;
	} else {
		char path[512] = {0};
		if (readlink("/proc/self/exe", path, sizeof(path) - 1) != -1) {
			modules_dir = std::string(dirname(path)) + "/modules/";
		} else {
			modules_dir = "modules/";
		}
	}

	DIR *plugins = opendir(modules_dir.c_str());

	// If we can't find the modules directory, we don't necessarily need to fail, so just
	// show a warning.
	if (!plugins) {
		LC_WARNING(LogModules) << "Unable to open modules directory: " << modules_dir;
		LC_WARNING(LogModules) << "*** NO MODULES LOADED";
		return true;
	}

	// Enumerate through the directory, ignoring files that begin with a dot.
	while (!!(dirent = readdir(plugins))) {
		if (dirent->d_name[0] == '.')
			continue;

		// Build the full path, and attempt to load the module.
		std::stringstream full_path;
		full_path << modules_dir << "/" << dirent->d_name;
		if (!LoadModule(full_path.str()))
			goto rollback;
	}

	// Finish enumerating.
	closedir(plugins);
	return true;

rollback:
	closedir(plugins);
	UnloadModules();
	return false;
}

bool ModuleManager::LoadModule(std::string filename)
{
	Module *module = new Module(filename);

	// Create a module descriptor, and load it.
	LC_DEBUG1(LogModules) << "Attempting to load module from " << filename;
	if (!module->Load()) {
		LC_ERROR(LogModules) << "Loading module " << filename << " failed.";
		delete module;
		return false;
	}

	LoadedModules.push_back(module);

	return true;
}

void ModuleManager::UnloadModules()
{
	for (auto module : LoadedModules) {
		module->Unload();
		delete module;
	}

	LoadedModules.clear();
}
