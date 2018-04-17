/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StateBlock.h
 * Author: harry
 *
 * Created on 10 April 2018, 15:42
 */

#ifndef STATEBLOCK_H
#define STATEBLOCK_H

#include <cassert>
#include <string>
#include <vector>
#include <map>

namespace archsim {
		
		class StateBlockInstance;
		
		/**
		 * The State Block is essentially a contiguous key-value store used
		 * for various metadata and metadata pointers. During Thread 
		 * initialisation, different parts of the simulation can allocate 
		 * memory within the state block which can then be used later on 
		 * during simulation. 
		 * 
		 * This way, JIT systems can keep a pointer to the state block and
		 * still be able to quickly look up information by using the block
		 * offsets.
		 * 
		 * The State Block is split into a descriptor and instance, since
		 * you might have multiple systems using the exact same configuration.
		 * However, right now this isn't managed automatically and so a new
		 * descriptor and state block is created for each guest thread.
		 */
		class StateBlockDescriptor {
		public:
			StateBlockDescriptor() : total_size_(0), finalised_(0) {}
			
			void AddBlock(const std::string &name, size_t size_in_bytes);
			size_t GetBlockOffset(const std::string &name);
			
			size_t GetSizeInBytes() { assert(finalised_); return total_size_; }
			
			StateBlockInstance *GetNewInstance();
			void Finalise() { finalised_ = true; }
			
		private:
			std::map<std::string, uint64_t> block_offsets_;
			size_t total_size_;
			bool finalised_;
		};
		
		class StateBlockInstance {
		public:
			StateBlockInstance(StateBlockDescriptor &descriptor) : descriptor_(descriptor), data_() { descriptor_.Finalise(); data_.resize(descriptor_.GetSizeInBytes()); }
			
			const StateBlockDescriptor &GetDescriptor() const { return descriptor_; }
			void *GetData() { return data_.data(); }
			
		private:
			StateBlockDescriptor &descriptor_;
			std::vector<unsigned char> data_;
		};
		
}

#endif /* STATEBLOCK_H */

