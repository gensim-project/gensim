/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ProcessorStateInterface.h
 * Author: harry
 *
 * Created on 10 April 2018, 13:47
 */

#ifndef THREADINSTANCE_H_
#define THREADINSTANCE_H_

#include "gensim/StateBlock.h"
#include "abi/Address.h"
#include "gensim/ArchDescriptor.h"

#include <libtrace/TraceSource.h>

namespace archsim {
		class MemoryInterface;
		class FeatureState;
		class FPState;
		
		/**
		 * A ProcessorInstance represents a single instance of a guest thread.
		 * It contains all data on registers, memory interfaces, etc., but no
		 * code to actually perform any execution.
		 * 
		 * However, it does contain the state block which is used to contain
		 * metadata for various other parts of the execution (in order to 
		 * keep the entire thread instance self contained).
		 */
		class ThreadInstance {
		public:
			using memory_interface_collection_t = std::map<std::string, MemoryInterface*>;
			
			ThreadInstance(const ArchDescriptor &arch, StateBlockDescriptor &state_descriptor);
			
			void *GetRegisterFile() { return (void*)register_file_.data(); }
			MemoryInterface &GetMemoryInterface(const std::string &interface_name);
			FeatureState &GetFeatures();
			FPState &GetFPState();
			
			uint32_t GetModeID() const { return mode_id_; }
			void SetModeID(uint32_t new_mode) { mode_id_ = new_mode; }
			
			Address GetTaggedSlot(const std::string &tag);
			void SetTaggedSlot(const std::string &tag, Address value);
			
			uint32_t GetExecutionRing() const { return ring_id_; }
			void SetExecutionRing(uint32_t new_ring) { ring_id_ = new_ring; }
                        
			Address GetPC() { return GetTaggedSlot("PC"); }
			void SetPC(Address target) { SetTaggedSlot("PC", target); }
			Address GetSP() { return GetTaggedSlot("SP"); }
			void SetSP(Address target) { SetTaggedSlot("SP", target); }
			
			MemoryInterface &GetFetchMI();
			const memory_interface_collection_t &GetMemoryInterfaces() { return memory_interfaces_; }
			
			libtrace::TraceSource *GetTraceSource() { return trace_source_; }
			void SetTraceSource(libtrace::TraceSource *source) { trace_source_ = source; }
                        
			StateBlockInstance &GetStateBlock();
		private:
			const ArchDescriptor &descriptor_;
			memory_interface_collection_t memory_interfaces_;
			std::vector<unsigned char> register_file_;
			
			uint32_t mode_id_;
			uint32_t ring_id_;
			
			StateBlockInstance state_block_;
			libtrace::TraceSource *trace_source_;
		};
}

#endif /* PROCESSORSTATEINTERFACE_H */

