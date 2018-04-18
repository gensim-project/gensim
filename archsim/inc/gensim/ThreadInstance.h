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
#include "abi/devices/Component.h"
#include "gensim/ArchDescriptor.h"
#include "abi/EmulationModel.h"

#include <libtrace/TraceSource.h>

namespace archsim {
		class MemoryInterface;
		class FeatureState;
		
		enum class FlushMode {
			DoNotFlush,
			FlushToZero
		};
		enum class RoundingMode {
			RoundTowardZero
		};
		
		class FPState {
		public:
			void SetFlushMode(FlushMode newMode) { flush_mode_ = newMode; }
			FlushMode GetFlushMode() const { return flush_mode_; }
			
			void SetRoundingMode(RoundingMode newMode) { rounding_mode_ = newMode; }
			RoundingMode GetRoundingMode() const { return rounding_mode_; }
			
		private:
			RoundingMode rounding_mode_;
			FlushMode flush_mode_;
		};
		
		class RegisterFileInterface {
		public:
			RegisterFileInterface(const RegisterFileDescriptor &descriptor) : descriptor_(descriptor) { data_.resize(descriptor.GetSize()); }
			
			char *GetData() { return data_.data(); }
			
			template<typename T> T* GetEntry(const std::string &slotname) { return (T*)&data_[descriptor_.GetEntries().at(slotname).GetOffset()]; }
			
		private:
			const RegisterFileDescriptor &descriptor_;
			std::vector<char> data_;
		};
		
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
			
			ThreadInstance(const ArchDescriptor &arch, StateBlockDescriptor &state_descriptor, archsim::abi::EmulationModel &emu_model);
			
			const ArchDescriptor &GetArch() { return descriptor_; }
			void *GetRegisterFile() { return (void*)register_file_.GetData(); }
			RegisterFileInterface &GetRegisterFileInterface() { return register_file_; }
			MemoryInterface &GetMemoryInterface(const std::string &interface_name);
			FeatureState &GetFeatures();
			FPState &GetFPState() { return fp_state_; }
			
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
			
			archsim::abi::devices::CoreComponent *GetCoreDevice(uint32_t id) { return core_devices_.at(id); }
			void SetCoreDevice(uint32_t id, archsim::abi::devices::CoreComponent *dev) { core_devices_.insert({id, dev}); }
			
			StateBlockInstance &GetStateBlock();
			
			void fn_flush_itlb_entry(Address::underlying_t entry) {}
			void fn_flush_dtlb_entry(Address::underlying_t entry) {}
			
			archsim::abi::ExceptionAction TakeException(uint64_t category, uint64_t data);
			archsim::abi::EmulationModel &GetEmulationModel() { return emu_model_; }
		private:
			const ArchDescriptor &descriptor_;
			memory_interface_collection_t memory_interfaces_;
			RegisterFileInterface register_file_;
			FPState fp_state_;
			
			archsim::abi::EmulationModel &emu_model_;
			
			uint32_t mode_id_;
			uint32_t ring_id_;
			
			StateBlockInstance state_block_;
			libtrace::TraceSource *trace_source_;
			
			std::unordered_map<uint32_t, archsim::abi::devices::CoreComponent*> core_devices_;
		};
}

#endif /* PROCESSORSTATEINTERFACE_H */

