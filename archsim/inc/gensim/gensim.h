/*
 * File:   gensim.h
 * Author: s0803652
 *
 * Created on 17 October 2011, 16:32
 */

#ifndef _GENSIM_H
#define _GENSIM_H

#include <map>
#include <string>

namespace archsim
{
	namespace abi
	{
		class EmulationModel;
	}
	namespace ioc
	{
		class Context;
	}
	namespace sys
	{
		namespace mem
		{
			class Memory;
		}
	}
}

class System;
class CoreArch;
class MemoryModel;

namespace gensim
{

	struct ProcLibCtx {
		std::string procName;
		void* lib;
	};

	class Processor;

	class Gensim
	{
	public:
		static std::map<std::string, ProcLibCtx> loaded_libraries;
		static ProcLibCtx* LoadProcessorLibrary(std::string filename);
		static Processor* CreateProcessor(std::string proc_name, System& _sys, CoreArch& _core, archsim::ioc::Context& _ctx, archsim::abi::EmulationModel& emulation_model, int _core_id);
	};
}

#endif /* _GENSIM_H */
