/*
 * File:   session.h
 * Author: s0457958
 *
 * Created on 27 January 2015, 13:53
 */

#ifndef SESSION_H
#define	SESSION_H

#include "define.h"

#include "abi/devices/generic/Keyboard.h"
#include "abi/devices/generic/Mouse.h"
#include "module/ModuleManager.h"

namespace archsim
{
	class Session
	{
	public:
		abi::devices::generic::MouseAggregator global_mouse;
		abi::devices::generic::KeyboardAggregator global_kbd;
		
		module::ModuleManager &GetModuleManager() { return module_manager_; }
		
	private:
		module::ModuleManager module_manager_;
	};
}

#endif	/* SESSION_H */

