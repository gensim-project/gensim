/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ProcessorStateInterface.h
 * Author: harry
 *
 * Created on 10 April 2018, 13:47
 */

#ifndef THREADINSTANCE_H_
#define THREADINSTANCE_H_

#include "core/execution/ExecutionResult.h"
#include "core/thread/StateBlock.h"
#include "abi/Address.h"
#include "abi/devices/Component.h"
#include "core/arch/ArchDescriptor.h"
#include "abi/EmulationModel.h"
#include "abi/devices/PeripheralManager.h"
#include "core/thread/ProcessorFeatures.h"
#include "core/thread/ThreadMetrics.h"

#include <libtrace/TraceSource.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <setjmp.h>

#define CreateThreadExecutionSafepoint(thread) do { setjmp(thread->Safepoint); } while(0)

namespace archsim
{
	class MemoryInterface;
	class FeatureState;

	namespace abi
	{
		namespace devices
		{
			class CPUIRQLine;
			class IRQLine;
		}
	}

	namespace core
	{
		namespace thread
		{


			enum class ThreadMessage {
				Nop,
				Interrupt,
				Halt
			};

			enum class FlushMode {
				DoNotFlush,
				FlushToZero
			};
			enum class RoundingMode {
				RoundTowardZero
			};

			class FPState
			{
			public:
				void SetFlushMode(FlushMode newMode)
				{
					flush_mode_ = newMode;
				}
				FlushMode GetFlushMode() const
				{
					return flush_mode_;
				}

				void SetRoundingMode(RoundingMode newMode)
				{
					rounding_mode_ = newMode;
				}
				RoundingMode GetRoundingMode() const
				{
					return rounding_mode_;
				}

				void Apply();

			private:
				RoundingMode rounding_mode_;
				FlushMode flush_mode_;
			};

			class RegisterFileInterface
			{
			public:
				RegisterFileInterface(const RegisterFileDescriptor &descriptor) : descriptor_(descriptor)
				{
					data_.resize(descriptor.GetSize());
				}

				char *GetData()
				{
					return data_.data();
				}
				const char *GetData() const
				{
					return data_.data();
				}

				template<typename T> T* GetEntry(const std::string &slotname)
				{
					return (T*)&data_[descriptor_.GetEntries().at(slotname).GetOffset()];
				}

				template<typename T> T* GetTaggedSlotPointer(const std::string &tag)
				{
					return (T*)&data_[descriptor_.GetTaggedEntry(tag).GetOffset()];
				}
				Address GetTaggedSlot(const std::string &tag) const;
				void SetTaggedSlot(const std::string &tag, Address value);

			private:
				const RegisterFileDescriptor &descriptor_;
				std::vector<char> data_;
			};

			/**
			 * An exception indicating that something unusual happened during
			 * instruction execution.
			 */
			class ThreadException
			{
			public:
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
			class ThreadInstance
			{
			public:
				using memory_interface_collection_t = std::vector<MemoryInterface*>;

				ThreadInstance(util::PubSubContext &pubsub, const ArchDescriptor &arch, archsim::abi::EmulationModel &emu_model);

				// Functions to do with accessing the larger substructures within the thread
				const ArchDescriptor &GetArch() const
				{
					return descriptor_;
				}
				RegisterFileInterface &GetRegisterFileInterface()
				{
					return register_file_;
				}
				MemoryInterface &GetMemoryInterface(const std::string &interface_name);
				MemoryInterface &GetMemoryInterface(uint32_t id);
				ProcessorFeatureInterface &GetFeatures()
				{
					return features_;
				}
				const ProcessorFeatureInterface &GetFeatures() const
				{
					return features_;
				}
				FPState &GetFPState()
				{
					return fp_state_;
				}
				archsim::abi::devices::PeripheralManager &GetPeripherals()
				{
					return peripherals_;
				}
				StateBlock &GetStateBlock()
				{
					return state_block_;
				}
				const StateBlock &GetStateBlock() const
				{
					return state_block_;
				}
				archsim::abi::EmulationModel &GetEmulationModel()
				{
					return emu_model_;
				}

				// Functions to do with execution modes
				uint32_t GetModeID() const
				{
					return *(uint32_t*)(((char*)GetStateBlock().GetData()) + mode_offset_);
				}
				void SetModeID(uint32_t new_mode)
				{
					GetStateBlock().SetEntry<uint32_t>("ModeID", new_mode);
				}

				// Execution ring is very frequently accessed so it's necessary
				// to provide an optimised implementation.
				uint32_t GetExecutionRing() const
				{
					return *(uint32_t*)(((char*)GetStateBlock().GetData()) + ring_offset_);
				}
				void SetExecutionRing(uint32_t new_ring);

				// Functions to do with registers. These functions are quite slow
				// since they have to go via the register descriptor, so if using in a loop
				// it's recommended to use a faster method.
				void *GetRegisterFile()
				{
					return (void*)register_file_.GetData();
				}
				Address GetPC()
				{
					return GetRegisterFileInterface().GetTaggedSlot("PC");
				}
				void SetPC(Address target)
				{
					GetRegisterFileInterface().SetTaggedSlot("PC", target);
				}
				Address GetSP()
				{
					return GetRegisterFileInterface().GetTaggedSlot("SP");
				}
				void SetSP(Address target)
				{
					GetRegisterFileInterface().SetTaggedSlot("SP", target);
				}

				// Functions to do with memory interfaces
				MemoryInterface &GetFetchMI()
				{
					return *fetch_mi_;
				}
				const memory_interface_collection_t &GetMemoryInterfaces()
				{
					return memory_interfaces_;
				}

				// Functions to do with tracing
				libtrace::TraceSource *GetTraceSource()
				{
					return trace_source_;
				}
				void SetTraceSource(libtrace::TraceSource *source)
				{
					trace_source_ = source;
				}

				// External functions. I don't like that these are here.
				void fn_flush_itlb_entry(Address::underlying_t entry)
				{
					pubsub_.Publish(PubSubType::FlushTranslations, 0);
				}
				void fn_flush_dtlb_entry(Address::underlying_t entry)
				{
					UNIMPLEMENTED;
				}
				void fn_pgt_change()
				{
					UNIMPLEMENTED;
				}
				void fn_flush()
				{
					UNIMPLEMENTED;
				}
				void fn_mmu_notify_asid_change(uint32_t)
				{
					UNIMPLEMENTED;
				}
				void fn_mmu_notify_pgt_change()
				{
					UNIMPLEMENTED;
				}
				void fn_mmu_flush_all()
				{
					UNIMPLEMENTED;
				}
				void fn_mmu_flush_va(uint64_t)
				{
					UNIMPLEMENTED;
				}

				uint32_t fn___builtin_polymul8(uint8_t, uint8_t)
				{
					UNIMPLEMENTED;
				}
				uint32_t fn___builtin_polymul16(uint8_t, uint8_t)
				{
					UNIMPLEMENTED;
				}
				uint32_t fn___builtin_polymul64(uint8_t, uint8_t)
				{
					UNIMPLEMENTED;
				}

				void fn___builtin_cmpf32_flags(float a, float b)
				{
					UNIMPLEMENTED;
				}
				void fn___builtin_cmpf64_flags(double a, double b)
				{
					UNIMPLEMENTED;
				}
				void fn___builtin_cmpf32e_flags(float a, float b)
				{
					UNIMPLEMENTED;
				}
				void fn___builtin_cmpf64e_flags(double a, double b)
				{
					UNIMPLEMENTED;
				}

				int32_t fn___builtin_fcvt_f32_s32(float f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				int32_t fn___builtin_fcvt_f64_s32(double f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				int64_t fn___builtin_fcvt_f32_s64(float f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				int64_t fn___builtin_fcvt_f64_s64(double f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				uint32_t fn___builtin_fcvt_f32_u32(float f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				uint32_t fn___builtin_fcvt_f64_u32(double f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				uint64_t fn___builtin_fcvt_f32_u64(float f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				uint64_t fn___builtin_fcvt_f64_u64(double f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}

				float fn___builtin_f32_round(float f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}
				double fn___builtin_f64_round(double f, uint8_t mode)
				{
					UNIMPLEMENTED;
				}

				// Functions to do with manipulating state according to the architecture
				archsim::abi::ExceptionAction TakeException(uint64_t category, uint64_t data);
				archsim::abi::ExceptionAction TakeMemoryException(MemoryInterface &interface, Address address);
				archsim::abi::devices::IRQLine *GetIRQLine(uint32_t irq_no);

				// Record that an IRQ line is currently high
				void TakeIRQ();
				// Record that an IRQ line that was previously high has gone low
				void RescindIRQ();
				// Check for any acknowledged but still pending interrupts
				void PendIRQ();
				// Acknowledge an IRQ and take an interrupt
				void HandleIRQ();

				// Functions to do with sending messages to a running thread
				void SendMessage(ThreadMessage message)
				{
					std::unique_lock<std::mutex> lock(message_lock_);
					message_queue_.push(message);
					message_waiting_ = true;
				}

				bool HasMessage() const
				{
					return message_waiting_;
				}
				ThreadMessage GetNextMessage()
				{
					std::unique_lock<std::mutex> lock(message_lock_);
					auto message = message_queue_.front();
					message_queue_.pop();

					message_waiting_ = !message_queue_.empty();

					return message;
				}

				core::execution::ExecutionResult HandleMessage();

				util::PubSubscriber &GetPubsub()
				{
					return pubsub_;
				}
				archsim::core::thread::ThreadMetrics &GetMetrics()
				{
					return *metrics_;
				}

				// This safepoint stuff is super nasty. In theory it is used
				// similarly to C++ exceptions, but since we throw from within
				// JIT'd code we can't use 'proper' exceptions. So, instead,
				// we have to use setjmp/longjmp to emulate exceptions.
				jmp_buf Safepoint;
				void ReturnToSafepoint();
			private:
				const ArchDescriptor &descriptor_;
				memory_interface_collection_t memory_interfaces_;
				MemoryInterface *fetch_mi_;
				RegisterFileInterface register_file_;
				FPState fp_state_;
				archsim::abi::devices::PeripheralManager peripherals_;
				archsim::abi::EmulationModel &emu_model_;
				ProcessorFeatureInterface features_;
				util::PubSubscriber pubsub_;
				archsim::core::thread::ThreadMetrics *metrics_;

				uint32_t mode_offset_;
				uint32_t ring_offset_;

				std::mutex message_lock_;
				std::queue<ThreadMessage> message_queue_;
				bool message_waiting_;
				std::atomic<uint32_t> pending_irqs_;

				StateBlock state_block_;
				libtrace::TraceSource *trace_source_;

				std::vector<archsim::abi::devices::CPUIRQLine*> irq_lines_;

			};
		}
	}
}

#endif /* PROCESSORSTATEINTERFACE_H */

