/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Keyboard.h
 * Author: harry
 *
 * Created on 08 May 2018, 16:01
 */

#ifndef LIBGVNC_KEYBOARD_H
#define LIBGVNC_KEYBOARD_H

#include <stdint.h>
#include <functional>

namespace libgvnc {
	struct KeyEvent {
		bool Down;
		uint32_t KeySym;
	};
	
	using KeyboardCallback = std::function<void(const KeyEvent&)>;
	
	class Keyboard {
	public:		
		void SendEvent(struct KeyEvent &event) { callback_(event); }
		void SetCallback(const KeyboardCallback &callback) { callback_ = callback; }
		
	private:
		KeyboardCallback callback_;
	};
}

#endif /* KEYBOARD_H */

