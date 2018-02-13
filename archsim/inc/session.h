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

namespace archsim
{
	class Session
	{
	public:
		abi::devices::generic::MouseAggregator global_mouse;
		abi::devices::generic::KeyboardAggregator global_kbd;
	};
}

#endif	/* SESSION_H */

