/*
 * File:   ModuleManager.h
 * Author: s0457958
 *
 * Created on 18 February 2014, 13:54
 */

#ifndef MODULEMANAGER_H
#define	MODULEMANAGER_H

#include <list>
#include "util/system/SharedLibrary.h"

namespace archsim
{
	namespace util
	{
		class Module
		{
		public:
			Module(std::string filename);
			bool Load();
			void Unload();

		private:
			std::string filename;
			system::SharedLibrary shlib;
		};

		class ModuleManager
		{
		public:
			static bool LoadModules();
			static bool LoadModule(std::string filename);
			static void UnloadModules();

		private:
			static std::list<Module *> LoadedModules;
		};
	}
}

#endif	/* MODULEMANAGER_H */

