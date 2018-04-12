/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MemoryInterface.h
 * Author: harry
 *
 * Created on 10 April 2018, 16:10
 */

#ifndef MEMORYINTERFACE_H
#define MEMORYINTERFACE_H

#include "ArchDescriptor.h"

namespace archsim {
		
		/**
		 * This class represents a specific instantiation of a memory port
		 * on a thread.
		 */
		class MemoryInterface {
		public:
			MemoryInterface(const MemoryInterfaceDescriptor &descriptor) : descriptor_(descriptor) {}
			const MemoryInterfaceDescriptor &GetDescriptor() { return descriptor_; }

			
			
		private:
			const MemoryInterfaceDescriptor &descriptor_;
		};
		
}

#endif /* MEMORYINTERFACE_H */

