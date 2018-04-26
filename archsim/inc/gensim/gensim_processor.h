#ifndef __GENSIM_PROCESSOR_H
#define __GENSIM_PROCESSOR_H

#include <stdint.h>

#include <iomanip>
#include <sstream>

#include "define.h"
#include "system.h"
#include "translate/TranslationManager.h"

#include "abi/EmulationModel.h"
#include "abi/devices/IRQController.h"
//#include "abi/devices/MMU.h"
#include "abi/devices/PeripheralManager.h"
#include "abi/memory/MemoryModel.h"

#include "gensim/gensim_disasm.h"
#include "gensim/gensim_processor_state.h"
#include "gensim/gensim_processor_metrics.h"
#include "tracing/TraceManager.h"

#include "core/thread/ProcessorFeatures.h"

#include "util/Counter.h"
#include "util/CounterTimer.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"
#include "gensim_decode_context.h"

#include <atomic>
#include <stack>
#include <queue>
#include <setjmp.h>
#include <stdio.h>
#include <libtrace/TraceSource.h>

UseLogContext(LogCPU);
UseLogContext(LogIRQStack);

extern "C" struct mmap_ret_val DefaultMemAccessRead(struct cpuState *state, uint32_t virt_addr);
extern "C" struct mmap_ret_val DefaultMemAccessWrite(struct cpuState *state, uint32_t virt_addr);

#define CreateSafepoint() do {_safepoint_safe = true; LC_DEBUG2(LogCPU) << "Creating safepoint in " << __PRETTY_FUNCTION__; if(setjmp(_longjmp_safepoint)) { LC_DEBUG2(LogCPU) << "Returned to safepoint in " << __PRETTY_FUNCTION__; } } while(0)

namespace libtrace
{
	class TraceSource;
}

namespace archsim
{
	namespace abi
	{
		struct SignalData;
		namespace memory
		{
			class MemoryCounterEventHandler;
		}
	}
	namespace ij
	{
		class IJTranslationContext;
	}
	namespace translate
	{
		namespace llvm
		{
			class LLVMTranslationContext;
		}
	}
}

namespace gensim
{

	class BaseDecode;
	class BaseLLVMTranslate;
	class BaseIJTranslate;
	class BaseJumpInfo;
	class TraceManager;

	class RegisterBankDescriptor
	{
	public:
		RegisterBankDescriptor();
		RegisterBankDescriptor(const std::string &name, unsigned int offset_in_file, unsigned int number_of_regs_in_bank, unsigned int register_stride_in_bytes, unsigned int number_of_elements_in_register, unsigned int size_of_element_in_bytes, unsigned int stride_of_element_in_bytes, void *bank_data_start);

		const std::string &Name() const
		{
			return name_;
		}

		unsigned int OffsetInFile() const
		{
			return offset_in_file_;
		}
		unsigned int NumberOfRegistersInBank() const
		{
			return number_of_registers_in_bank_;
		}
		unsigned int RegisterStrideInBytes() const
		{
			return register_stride_in_bytes_;
		}
		unsigned int NumberOfElementsInRegister() const
		{
			return number_of_elements_in_register_;
		}
		unsigned int SizeOfElementInBytes() const
		{
			return size_of_element_in_bytes_;
		}
		unsigned int StrideOfElementInBytes() const
		{
			return stride_of_element_in_bytes_;
		}

		void *GetBankDataStart() const
		{
			return bank_data_start_;
		}
	private:
		std::string name_;
		unsigned int offset_in_file_;
		unsigned int number_of_registers_in_bank_;
		unsigned int register_stride_in_bytes_;
		unsigned int number_of_elements_in_register_;
		unsigned int size_of_element_in_bytes_;
		unsigned int stride_of_element_in_bytes_;

		void *bank_data_start_;
	};

	struct RegisterDescriptor {
		std::string Name;
		std::string Tag;
		int RegisterWidth;
		int Offset;
		void *DataStart;
	};

	class Processor
	{
	public:
		enum ExecMode {
			kExecModeInterpretive = 0x0, // interpretive execution mode
			kExecModeNative = 0x1, // native execution mode
		};

		Processor(const std::string &name, int _core_id, archsim::util::PubSubContext& pubsub);
		virtual ~Processor();

		const std::string &GetArchName() const
		{
			return arch_name_;
		}

		virtual bool Initialise(archsim::abi::EmulationModel& emulation_model, archsim::abi::memory::MemoryModel& memory_model);
		virtual void InitialiseFeatures() = 0;
		void Destroy();

		uint32_t GetCoreId() const
		{
			return core_id;
		}

		virtual void PrintStatistics(std::ostream& stream);

		void Halt();
		void InitialiseState();
		void DumpRegisters();

		virtual bool RunJIT(bool verbose, uint32_t steps);
		virtual bool RunInterp(uint32_t steps);
		virtual bool RunSingleInstruction();

		uint32_t mem_read_8(uint32_t addr, uint8_t &data);
		uint32_t mem_read_8s(uint32_t addr, uint8_t &data);
		uint32_t mem_read_8_user(uint32_t addr, uint8_t &data);
		uint32_t mem_read_16(uint32_t addr, uint16_t &data);
		uint32_t mem_read_16s(uint32_t addr, uint16_t &data);
		uint32_t mem_read_32(uint32_t addr, uint32_t &data);
		uint32_t mem_read_32_user(uint32_t addr, uint32_t &data);
		uint32_t mem_write_8(uint32_t addr, uint32_t data);
		uint32_t mem_write_8_user(uint32_t addr, uint32_t data);
		uint32_t mem_write_16(uint32_t addr, uint32_t data);
		uint32_t mem_write_32(uint32_t addr, uint32_t data);
		uint32_t mem_write_32_user(uint32_t addr, uint32_t data);

		virtual bool step_single() = 0;
		virtual bool step_single_fast() = 0;

		virtual bool step_block_fast() = 0;
		virtual bool step_block_trace() = 0;
		virtual bool step_block_fast_thread() = 0;

		virtual bool step_instruction(bool trace) = 0;

		virtual void reset() = 0;
		virtual bool reset_to_initial_state(bool) = 0;

		virtual uint32_t read_pc() = 0;
		virtual void write_pc(uint32_t) = 0;
		virtual uint32_t read_sp() = 0;
		virtual void write_sp(uint32_t) = 0;

		virtual void restart_from_halted() = 0;
		virtual void purge_dcode_cache() = 0;

		// todo: place a reference to the current instruction in this function
		virtual bool curr_insn_is_predicated() = 0;

		virtual gensim::BaseDecode *curr_interpreted_insn() = 0;

		virtual gensim::BaseLLVMTranslate *CreateLLVMTranslate(archsim::translate::llvm::LLVMTranslationContext& ctx) const = 0;

		virtual gensim::BaseDisasm *GetDisasm() = 0;
		virtual gensim::BaseDecode *GetNewDecode() = 0;

		virtual uint32_t DecodeInstr(uint32_t pc, uint8_t isa_mode, gensim::BaseDecode &target) = 0;
		virtual void DecodeInstrIr(uint32_t ir, uint8_t isa_mode, gensim::BaseDecode &target) = 0;

		virtual gensim::BaseJumpInfo *GetJumpInfo() const = 0;

		virtual void reset_exception();

		uint64_t instructions();

		void prepare_cpu();

		// An external device uses this function to assert an interrupt line
		void take_irq(archsim::abi::devices::CPUIRQLine* line);

		// An externcal device uses this function to rescind an interrupt
		void rescind_irq(archsim::abi::devices::CPUIRQLine* line);

		// This function is implemented by subclasses in order to actually handle the IRQs
		virtual void handle_irq(uint32_t line);

		// This function implements the logic for selecting an interrupt to handle, then calling the handler
		void handle_pending_irq();

		virtual void handle_fetch_fault(uint32_t fault) = 0;

		archsim::abi::devices::IRQLine* GetIRQLine(uint32_t line);


		// TLB Flushing Functions
		void flush_tlb();
		void flush_itlb();
		void flush_dtlb();
		void flush_itlb_entry(virt_addr_t virt_addr);
		void flush_dtlb_entry(virt_addr_t virt_addr);

		inline bool has_pending_actions() const
		{
			if (state.pending_actions) {
				LC_DEBUG2(LogCPU) << "Pending action detected! " << state.pending_actions;
			}
			return state.pending_actions != 0;
		}

		inline uint32_t handle_pending_action()
		{
			if (state.pending_actions)
				metrics.interrupts_serviced.inc();

			if (state.pending_actions & PENDINGACTION_INTERRUPT) {
				LC_DEBUG2(LogCPU) << "Handling pending IRQ";
//				handle_pending_irq();
				UNIMPLEMENTED;
				return 0;
			} else if (state.pending_actions & PENDINGACTION_SIGNAL) {
				handle_pending_signal();
				return 0;
			} else if (state.pending_actions & PENDINGACTION_ABORT) {
				return 1;
			}

			return 1;
		}

		inline void pend_interrupt()
		{
			bool should_pend_interrupts = false;
			for (auto irq : irq_lines_) {
				if (irq->IsAsserted()) {
					should_pend_interrupts = true;
					break;
				}
			}

			if (should_pend_interrupts) {
				state.pending_actions |= PENDINGACTION_INTERRUPT;

				for (auto irq : irq_lines_) {
					irq->Raise();
				}
			}
		}

		inline archsim::abi::ExceptionAction take_exception(uint32_t category, uint32_t data)
		{
			UNIMPLEMENTED;
//			last_exception_action = GetEmulationModel().HandleException(*this, category, data);
//			if(last_exception_action == archsim::abi::AbortInstruction || last_exception_action == archsim::abi::AbortSimulation) {
//				LC_DEBUG4(LogCPU) << "Take Exception";
//				return_to_safepoint();
//			}
//			return last_exception_action;
		}

		void SetMustLeave()
		{
			leave_jit = 1;
		}

		void ClearMustLeave()
		{
			leave_jit = 0;
		}

		virtual void handle_exception(uint32_t category, uint32_t data);

		inline RegisterBankDescriptor &GetRegisterBankDescriptor(std::string name)
		{
			return register_banks.at(name);
		}

		size_t GetRegisterBankCount() const
		{
			return register_banks.size();
		}

		inline const RegisterDescriptor &GetRegisterDescriptor(std::string name) const
		{
			return registers.at(name);
		}

		inline size_t GetRegisterCount() const
		{
			return registers.size();
		}

		inline RegisterBankDescriptor &GetRegisterBankDescriptor(uint32_t id)
		{
			return register_banks_by_id.at(id);
		}

		inline RegisterDescriptor &GetRegisterDescriptor(uint32_t id)
		{
			return registers_by_id.at(id);
		}

		inline RegisterDescriptor *GetRegisterByTag(const std::string &tag)
		{
			for(auto &i : registers) {
				if(i.second.Tag == tag) return &i.second;
			}
			return nullptr;
		}

		inline const RegisterDescriptor *GetRegisterByTag(const std::string &tag) const
		{
			for(auto &i : registers) {
				if(i.second.Tag == tag) return &i.second;
			}
			return nullptr;
		}

		inline void *GetRegisterFilePointer()
		{
			return (void*)state.gensim_state;
		}

		virtual uint32_t GetRegisterFileSize() = 0;

		inline void trap()
		{
#if defined(__x86_64__)
			asm("int3\n");
#elif defined(__arm__)
			asm("bkpt\n");
#else
#error "Unsupported architecture"
#endif
		}

		inline void set_cpu_mode(uint8_t new_mode)
		{
			state.isa_mode = new_mode;
		}

		inline uint8_t get_cpu_mode() const
		{
			return state.isa_mode;
		}

		inline uint32_t build_mmu_access_word(uint8_t is_write)
		{
			return in_kernel_mode() | (is_write << 1);
		}

		inline bool in_kernel_mode()
		{
			return state.in_kern_mode;
		}

		inline void enter_kernel_mode()
		{
			if(state.in_kern_mode) return;

			state.in_kern_mode = 1;
			GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::PrivilegeLevelChange, this);
		}

		inline void enter_user_mode()
		{
			if(!state.in_kern_mode) return;

			state.in_kern_mode = 0;
			GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::PrivilegeLevelChange, this);
		}

		inline uint8_t probe_device(uint32_t device_id)
		{
			if (peripherals.CoreProbeDevice(device_id)) return 1;
			//LC_ERROR(LogCPU) << "Probe of coprocessor " << device_id << " failed";
			return 0;
		}

		inline bool write_device(uint32_t device_id, uint32_t addr, uint32_t data)
		{
			return peripherals.CoreWriteDevice(device_id, addr, data);
		}

		inline bool read_device(uint32_t device_id, uint32_t addr, uint32_t &data)
		{
			return peripherals.CoreReadDevice(device_id, addr, data);
		}

		bool halted;

		uint16_t core_id;

		cpuState state; // cpu state structure

		ExecMode cur_exec_mode;

		bool end_of_block; // true if end of basic block reached
		bool static_insn_predication; // true if an instruction can be determined to be predicated at decode

		// When an interrupt or exception is entered, that state is pushed on
		// the interrupt_stack data structure. Thus the top-of-stack always denotes
		// the current InterruptState.
		//
		uint32_t interrupt_mode;

		void push_interrupt(uint32_t i)
		{
			interrupt_mode = i;
		}

		void pop_interrupt()
		{
		}

		archsim::util::CounterTimer exec_time; // target binary exeuction time
		archsim::util::CounterTimer trace_time; // trace construction time
		archsim::util::CounterTimer native_time;
		archsim::util::CounterTimer interp_time;
		archsim::util::CounterTimer compile_time;

		gensim::processor::ProcessorMetrics metrics;

		archsim::abi::devices::PeripheralManager peripherals;

		inline void assert_abort_action()
		{
			state.pending_actions |= PENDINGACTION_ABORT;
		}

		inline void assert_signal(int signum)
		{
			pending_signals.push(signum);
			state.pending_actions |= PENDINGACTION_SIGNAL;
		}

		inline void handle_pending_signal()
		{
			int signum = pending_signals.front();
			pending_signals.pop();
			if (pending_signals.size() == 0)
				state.pending_actions &= ~PENDINGACTION_SIGNAL;

			archsim::abi::SignalData *data;
			if (GetEmulationModel().GetSignalData(signum, data)) {
				LC_DEBUG2(LogCPU) << "Attempting to invoke signal " << signum << " on emulation model";
				GetEmulationModel().InvokeSignal(signum, read_pc(), data);
			}
		}

		inline archsim::abi::EmulationModel& GetEmulationModel() const
		{
			return *emulation_model;
		}

		inline archsim::abi::memory::MemoryModel& GetMemoryModel() const
		{
			return *memory_model;
		}

		inline bool HasTraceManager() const
		{
			return trace_mgr != NULL;
		}

		inline bool IsTracingEnabled() const
		{
			return tracing_enabled;
		}

		inline libtrace::TraceSource *GetTraceManager() const
		{
			return trace_mgr;
		}

		inline archsim::abi::memory::Endianness GetEndianness() const
		{
			return endianness;
		}
		inline void SetEndianness(archsim::abi::memory::Endianness new_val)
		{
			endianness = new_val;
		}

	protected:
		archsim::abi::ExceptionAction last_exception_action;

		std::unordered_map<std::string, RegisterBankDescriptor> register_banks;
		std::unordered_map<std::string, RegisterDescriptor> registers;
		std::map<uint32_t, RegisterBankDescriptor> register_banks_by_id;
		std::map<uint32_t, RegisterDescriptor> registers_by_id;

		std::queue<int> pending_signals;

	public:
		bool InitialiseTracing();
		void DestroyTracing();

		virtual void StartTracing()
		{
			if(tracing_enabled) return;
			if(!trace_mgr) InitialiseTracing();
			tracing_enabled = true;
			GetSubscriber().Publish(PubSubType::FlushTranslations, NULL);
		}
		virtual void StopTracing()
		{
			tracing_enabled = false;
			GetSubscriber().Publish(PubSubType::FlushTranslations, NULL);
		}

		void return_to_safepoint();

		archsim::ProcessorFeatureInterface &GetFeatures()
		{
			return _features;
		}
		const archsim::ProcessorFeatureInterface &GetFeatures() const
		{
			return _features;
		}

	private:
		std::string arch_name_;

		bool tracing_enabled;
		bool leave_jit;

		libtrace::TraceSource *trace_mgr;
		archsim::abi::memory::MemoryModel *memory_model;
		archsim::abi::EmulationModel *emulation_model;
		archsim::abi::memory::MemoryCounterEventHandler *mem_counter;
		archsim::abi::memory::Endianness endianness;

		archsim::util::PubSubscriber subscriber;

		gensim::DecodeContext *decode_ctx_;

		archsim::ProcessorFeatureInterface _features;

		// A (potentially sparse) map of connected IRQ lines
		std::vector<archsim::abi::devices::CPUIRQLine*> irq_lines_;
		std::atomic<uint32_t> asserted_irqs;


	protected:
		jmp_buf _longjmp_safepoint;
		bool _safepoint_safe;

		bool _skip_verify;

		archsim::util::PubSubscriber &GetSubscriber()
		{
			return subscriber;
		}
		bool JitInterpBlock(virt_addr_t pc, bool verbose);

	};
}

#endif
