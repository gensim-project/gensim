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
		
		class StateBlock;
		
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
		 */
	
		class StateBlock {
		public:			
			void AddBlock(const std::string &name, size_t size_in_bytes);
			size_t GetBlockOffset(const std::string &name) const;
			
			void *GetData() { return data_.data(); }
			
			template<typename T> T* GetEntry(const std::string &entryname) { return (T*)(data_.data() + GetBlockOffset(entryname)); }
			
		private:
			std::map<std::string, uint64_t> block_offsets_;
			std::vector<unsigned char> data_;
		};
		
}

#endif /* STATEBLOCK_H */

