/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * VirtualScreenManager.h
 *
 *  Created on: 21 Jul 2014
 *      Author: harry
 */

#ifndef VIRTUALSCREENMANAGER_H_
#define VIRTUALSCREENMANAGER_H_

#include "util/LogContext.h"

#include <unordered_map>

UseLogContext(LogVSM)

class System;
namespace archsim
{


	namespace abi
	{

		namespace memory
		{
			class MemoryModel;
		}

		namespace devices
		{

			namespace gfx
			{

				class VirtualScreen;
				class SDLScreen;

				class VirtualScreenManagerBase
				{
				public:
					virtual ~VirtualScreenManagerBase();

					virtual VirtualScreen *CreateScreenInstance(std::string screen_id, memory::MemoryModel *memory, System *sys) = 0;
					virtual VirtualScreen *GetVirtualScreenInstance(std::string screen_id) = 0;
					virtual void DeleteVirtualScreenInstance(std::string screen_id) = 0;
				};

				template<class ScreenInstanceT> class VirtualScreenManager : public VirtualScreenManagerBase
				{

				private:
					std::unordered_map<std::string, VirtualScreen*> Screens;
				public:
					~VirtualScreenManager()
					{
						for(auto a : Screens) delete a.second;
					}

					VirtualScreen *CreateScreenInstance(std::string screen_id, memory::MemoryModel *memory, System *sys) override
					{
						ScreenInstanceT *new_screen = new ScreenInstanceT(screen_id, memory, sys);
						Screens[screen_id] = new_screen;
						return (VirtualScreen*)new_screen;
					}

					VirtualScreen *GetVirtualScreenInstance(std::string screen_id) override
					{
						if(Screens.count(screen_id)) return (VirtualScreen*)Screens.at(screen_id);
						return NULL;
					}

					void DeleteVirtualScreenInstance(std::string screen_id) override
					{
						ScreenInstanceT *screen = (ScreenInstanceT*)GetVirtualScreenInstance(screen_id);
						delete screen;
						Screens.erase(screen_id);
					}
				};


			}

		}
	}
}



#endif /* VIRTUALSCREENMANAGER_H_ */
