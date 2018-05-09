/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Pointer.h
 * Author: harry
 *
 * Created on 08 May 2018, 16:01
 */

#ifndef LIBGVNC_POINTER_H
#define LIBGVNC_POINTER_H

#include <stdint.h>
#include <functional>

namespace libgvnc {
	
	struct PointerEvent {
		uint8_t ButtonMask;
		uint16_t PositionX;
		uint16_t PositionY;
	};
	
	using PointerCallback = std::function<void(struct PointerEvent&)>;
	
	class Pointer {
	public:
		void SendEvent(struct PointerEvent &event) { callback_(event); }
		void SetCallback(const PointerCallback &callback) { callback_ = callback; }
		
	private:
		PointerCallback callback_;
	};
}

#endif /* POINTER_H */

