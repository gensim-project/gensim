/*
 * gensim/gensim_processor.cpp
 */
#include "system.h"

#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "abi/memory/system/SystemMemoryTranslationModel.h"
#include "abi/memory/MemoryCounterEventHandler.h"

#include "gensim/gensim_processor.h"
#include "gensim/gensim_decode.h"

#include "tracing/TraceManager.h"
#include "translate/TranslationManager.h"

#include "util/Counter.h"
#include "util/MultiHistogram.h"
#include "util/Histogram.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"
#include "util/ComponentManager.h"
#include "uarch/uArch.h"
#include "uarch/cache/CacheGeometry.h"

#include <libtrace/ArchInterface.h>
#include <libtrace/TraceSink.h>

#include <cassert>
#include <iomanip>
#include <map>
#include <sys/mman.h>
#include <libtrace/TraceSource.h>

DeclareLogContext(LogCPU, "CPU");
DeclareChildLogContext(LogIRQ, LogCPU, "IRQ");
DeclareChildLogContext(LogIRQStack, LogIRQ, "Stack");

DefineComponentType(gensim::Processor, const std::string&, int, archsim::util::PubSubContext*);

using namespace gensim;

RegisterBankDescriptor::RegisterBankDescriptor() {}

RegisterBankDescriptor::RegisterBankDescriptor(const std::string& name, unsigned int offset_in_file, unsigned int number_of_regs_in_bank, unsigned int register_stride_in_bytes, unsigned int number_of_elements_in_register, unsigned int size_of_element_in_bytes, unsigned int stride_of_element_in_bytes, void *bank_data_start)
	: name_(name), offset_in_file_(offset_in_file), number_of_registers_in_bank_(number_of_regs_in_bank), register_stride_in_bytes_(register_stride_in_bytes), number_of_elements_in_register_(number_of_elements_in_register), size_of_element_in_bytes_(size_of_element_in_bytes), stride_of_element_in_bytes_(stride_of_element_in_bytes), bank_data_start_(bank_data_start)
{

}


Processor::Processor(const std::string &arch_name, int _core_id, archsim::util::PubSubContext& pubsub)
	: asserted_irqs(0),
	  halted(false),
	  core_id(_core_id),
	  memory_model(NULL),
	  cur_exec_mode(kExecModeInterpretive),
	  peripherals(*(archsim::ThreadInstance*)0),
	  interrupt_mode(0),
	  emulation_model(NULL),
	  trace_mgr(NULL),
	  mem_counter(NULL),
	  endianness(archsim::abi::memory::LittleEndian),
	  tracing_enabled(false),
	  end_of_block(false),
	  last_exception_action(archsim::abi::None),
	  subscriber(pubsub),
	  _skip_verify(false),
	  _safepoint_safe(false),
	  _features(pubsub),
	  arch_name_(arch_name)
{
	LC_DEBUG1(LogCPU) << "Constructing processor";

	// Clear the CPU state structure.
	bzero(&state, sizeof(state));
}

Processor::~Processor()
{
	_safepoint_safe = false;
	LC_DEBUG1(LogCPU) << "Deleting processor";
}

static void flush_icache_callback(PubSubType::PubSubType type, void *context, const void *data)
{
	assert(type == PubSubType::L1ICacheFlush);
	gensim::Processor *cpu = (gensim::Processor *)context;

	cpu->purge_dcode_cache();
}

bool Processor::Initialise(archsim::abi::EmulationModel& emulation_model, archsim::abi::memory::MemoryModel& memory_model)
{
	LC_DEBUG1(LogCPU) << "Initialising processor";

	if(!GetComponentInstance(GetArchName(), decode_ctx_, this)) {
		LC_ERROR(LogCPU) << "Could not find decode context for arch \"" << GetArchName() << "\"";
		return false;
	}

	this->emulation_model = &emulation_model;
	this->memory_model = &memory_model;

	// If memory event counting is enabled, activate this now.
	if (archsim::options::MemEventCounting) {
		mem_counter = new archsim::abi::memory::MemoryCounterEventHandler();
		memory_model.RegisterEventHandler(*mem_counter);
	}

	// Install the cache geometry into the memory model -- if requested.
	if (archsim::options::CacheModel) {
		emulation_model.GetuArch().GetCacheGeometry().Install(*this, memory_model);
	}

	if (archsim::options::Trace) {
		if (!InitialiseTracing())
			return false;

		StartTracing();
	}

	InitialiseFeatures();

	// Initialise processor state structure
	InitialiseState();

	return true;
}

bool Processor::InitialiseTracing()
{
	assert(!trace_mgr);

	trace_mgr = new libtrace::TraceSource(1000000);
	libtrace::TraceSink *sink = nullptr;
	FILE *file;
	
	if(archsim::options::TraceFile.IsSpecified()) {
		file = fopen(archsim::options::TraceFile.GetValue().c_str(), "w");
		
		if(file == nullptr) {
			LC_ERROR(LogCPU) << "Could not open trace file for writing";
			return false;
		}
	} else {
		file = stdout;
	}
	
	if(archsim::options::TraceMode.GetValue() == "text") {
		LC_ERROR(LogCPU) << "Text-mode tracing not currently supported";
		return false;
//		sink = new libtrace::TextFileTraceSink(file, new libtrace::DefaultArchInterface());
	} else if(archsim::options::TraceMode.GetValue() == "binary") {
		if(!archsim::options::TraceFile.IsSpecified()) {
			LC_ERROR(LogCPU) << "Cannot use binary tracing unless a file is specified. Please use --trace-file to specify an output file.";
			return false;
		}
		
		sink = new libtrace::BinaryFileTraceSink(file);
	} else {
		LC_ERROR(LogCPU) << "Unknown trace mode " << archsim::options::TraceMode.GetValue();
		return false;
	}

	assert(sink != nullptr);

	trace_mgr->SetSink(sink);

	return true;
}

void Processor::DestroyTracing()
{
	assert(trace_mgr);

	trace_mgr->Flush();
	trace_mgr->Terminate();
	delete trace_mgr;
	trace_mgr = NULL;
}

void Processor::Destroy()
{
	LC_DEBUG1(LogCPU) << "Destroying processor";

	if (trace_mgr) {
		DestroyTracing();
	}

	if (mem_counter) {
		delete mem_counter;
	}
}

/**
 * Initialises the CPU state structure
 */
void Processor::InitialiseState()
{
	// Wire-up pointer to surrounding cpuContext on cpuState struct
	state.cpu_ctx = (void *)this;

	// Store the JIT region entry table
	if (GetEmulationModel().GetSystem().HaveTranslationManager()) {
		state.jit_entry_table = (jit_region_fn_table_ptr)GetEmulationModel().GetSystem().GetTranslationManager().GetRegionTxlnCachePtr();
	}

	// Initialise the ISA mode with the default value.
	state.isa_mode = 0;

	// Initialise iteration counter.
	state.iterations = 0;

	// Clear pending actions
	state.pending_actions = 0;
}

void Processor::reset()
{
	// Prepare CPU
	prepare_cpu();
}

bool Processor::reset_to_initial_state(bool purge_translations)
{
	// Initialise processor state structure
	InitialiseState();

	// Reset CPU
	reset();

	// Reset timing counters
	exec_time.Reset();
	trace_time.Reset();

	// Purge decode caches
	purge_dcode_cache();

	return true;
}

void Processor::prepare_cpu()
{
	// Empty and set-up interrupt state
	interrupt_mode = 0;

	end_of_block = false;
}

void Processor::reset_exception()
{
	LC_WARNING(LogCPU) << "Unhandled guest CPU reset exception";
	assert(!"Reset exception asserted on a core with no reset handler");
}

void Processor::handle_exception(uint32_t category, uint32_t data)
{
	LC_WARNING(LogCPU) << "Unhandled guest CPU exception category=" << category << ", data=" << data;
}


void Processor::flush_tlb()
{
//	if (GetEmulationModel().GetSystem().HaveTranslationManager())
//		GetEmulationModel().GetSystem().GetTranslationManager().InvalidateRegionTxlnCache();

	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);
	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);

	purge_dcode_cache();
}

void Processor::flush_itlb()
{
	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::ITlbFullFlush, 0);

	purge_dcode_cache();
}

void Processor::flush_dtlb()
{
	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::DTlbFullFlush, 0);
}

void Processor::flush_itlb_entry(virt_addr_t virt_addr)
{
	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::ITlbEntryFlush, (void*)(uint64_t)virt_addr);
}

void Processor::flush_dtlb_entry(virt_addr_t virt_addr)
{
	GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::DTlbEntryFlush, (void*)(uint64_t)virt_addr);
}

void Processor::return_to_safepoint()
{
	LC_DEBUG1(LogCPU) << "Returned to safepoint";
	assert(_safepoint_safe);
	longjmp(_longjmp_safepoint, 0);
}

// Interpretive simulation mode without instruction tracing --------------------
bool Processor::RunInterp(uint32_t iterations)
{
	bool stepOK = true;

	state.iterations = iterations;

	// Record simulation start time
	if (archsim::options::Verbose)
		exec_time.Start();

//	make_safepoint();

	CreateSafepoint();

	if(GetTraceManager() != NULL && GetTraceManager()->IsPacketOpen()) GetTraceManager()->Trace_End_Insn();

	assert(!GetTraceManager() || !GetTraceManager()->IsPacketOpen());

	if (GetEmulationModel().GetSystem().HasBreakpoints()) {
		while (stepOK && !halted && state.iterations--) {
			if (GetEmulationModel().GetSystem().ShouldBreak(read_pc())) {
				LC_DEBUG1(LogCPU) << "Hit a user-specified breakpoint";
				GetEmulationModel().GetSystem().SetSimulationMode(System::SingleStep);
				return true;
			}

			if (IsTracingEnabled()) {
				stepOK = step_single();
			} else {
				stepOK = step_single_fast();
			}
		}
	} else if (IsTracingEnabled()) {
		while (stepOK && !halted && state.iterations--) {
			stepOK = step_block_trace();

			if(state.pending_actions) {
				handle_pending_action();
			}
		}
	} else {
		while (stepOK && !halted && state.iterations--) {
			stepOK = step_block_fast();

			if(state.pending_actions) {
				handle_pending_action();
			}
		}
	}

	// Record and update simulation end time
	if (archsim::options::Verbose)
		exec_time.Stop();

	return stepOK;
}

bool Processor::RunSingleInstruction()
{
	bool stepOK = true;

	state.iterations = 1;

	// Record simulation start time
	if (archsim::options::Verbose)
		exec_time.Start();

	stepOK = step_instruction(!!trace_mgr);

	// Record and update simulation end time
	if (archsim::options::Verbose)
		exec_time.Stop();

	return stepOK;
}

std::mutex m;

//#define IRQ_LATENCY
#ifdef IRQ_LATENCY
std::chrono::high_resolution_clock::time_point then;
uint64_t trig, last_trig;
#endif

void Processor::take_irq(archsim::abi::devices::CPUIRQLine* irq_line)
{
	std::lock_guard<std::mutex> l(m);

	LC_DEBUG2(LogIRQ) << "IRQ line " << std::hex << irq_line << " asserted";
	if(asserted_irqs == 0) {
		LC_DEBUG2(LogIRQ) << "IRQs now pending";
	}

	//Only actually assert an interrupt if we aren't reasserting one we've already ack'd
	if(!irq_line->IsAcknowledged()) {
#ifdef IRQ_LATENCY
		then = std::chrono::high_resolution_clock::now();
		trig++;
#endif

		asserted_irqs++;
		state.pending_actions |= PENDINGACTION_INTERRUPT;
	}
}

void Processor::rescind_irq(archsim::abi::devices::CPUIRQLine *irq_line)
{
	std::lock_guard<std::mutex> l(m);

	LC_DEBUG2(LogIRQ) << "IRQ line " << std::hex << irq_line << " rescinded";
	if(!irq_line->IsAsserted()) {
		LC_DEBUG2(LogIRQ) << "Rescinded an IRQ line which was not asserted";
	}

	if(asserted_irqs == 0) {
		LC_DEBUG2(LogIRQ) << "Rescinded an IRQ line when no interrupts were pending";
		return;
	}

	if ((--asserted_irqs) == 0) {
		LC_DEBUG1(LogIRQ) << "IRQs no longer pending";
		state.pending_actions &= ~PENDINGACTION_INTERRUPT;
	}
}

archsim::abi::devices::IRQLine *Processor::GetIRQLine(uint32_t line_no)
{
	for(auto line : irq_lines_) {
		if(line->Line() == line_no) {
			return line;
		}
	}
	UNIMPLEMENTED;
//	auto new_line = new archsim::abi::devices::CPUIRQLine(this);
//	new_line->SetLine(line_no);
//	irq_lines_.push_back(new_line);
//	return new_line;
}

void Processor::handle_irq(uint32_t line)
{
	assert(!"No implementation of handle_irq provided in processor model");
}

void Processor::handle_pending_irq()
{
#ifdef IRQ_LATENCY
	if (trig > last_trig) {
		last_trig = trig;

		auto now = std::chrono::high_resolution_clock::now();
		fprintf(stderr, "LATENCY: %lu\n", std::chrono::duration_cast<std::chrono::nanoseconds>(now - then).count());
	}
#endif

	if(!asserted_irqs) {
		LC_DEBUG2(LogIRQ) << "An IRQ was pended, but none was asserted";
		state.pending_actions &= ~PENDINGACTION_INTERRUPT;
	}
	for(const auto &irq : irq_lines_) {
		if(irq->IsAsserted() && !irq->IsAcknowledged()) {
			LC_DEBUG1(LogIRQ) << "Handling pending IRQ on line " << (uint32_t)irq->Line() << ", insn " << metrics.interp_inst_count.get_value() << ", pc " << std::hex << read_pc();
			handle_irq(irq->Line());
			LC_DEBUG1(LogIRQ) << "Handled pending IRQ on line " << (uint32_t)irq->Line() << ", insn " << metrics.interp_inst_count.get_value() << ", pc " << std::hex << read_pc();
			irq->Acknowledge();
			return;
		}
	}
	//We've got to here without hitting an interrupt which we actually take, so all interrupts are deasserted or acknowledged
	state.pending_actions &= ~PENDINGACTION_INTERRUPT;
}

void Processor::Halt()
{
	assert_abort_action();

	halted = true;
}

uint64_t Processor::instructions()
{
	return metrics.interp_inst_count.get_value() + metrics.native_inst_count.get_value();
}

/**
 * Dumps processor statistics to the console
 */
void Processor::PrintStatistics(std::ostream& stream)
{
	
	if (archsim::options::ProfilePcFreq) {
		std::ofstream pc_freq ("pc_freq.out");
		pc_freq << "Instruction frequency histogram" << std::endl;
		pc_freq << "----------------------------------------" << std::endl;
		auto &hist = metrics.pc_freq_hist;

		for(auto entry : hist.get_value_map()) {
			pc_freq << std::hex << entry.first << "\t" << std::dec << *entry.second << std::endl;
		}
	}

	if (archsim::options::ProfileIrFreq) {
		std::ofstream ir_freq ("ir_freq.out");

		ir_freq << "Instruction IR frequency histogram" << std::endl;
		ir_freq << "-----------------------------------------" << std::endl;
		auto &hist = metrics.inst_ir_freq_hist;

		for(auto entry : hist.get_value_map()) {
			ir_freq << std::hex << std::setw(8) << std::setfill('0') << entry.first << "\t" << std::dec << *entry.second << std::endl;
		}
	}

	if (archsim::options::Profile) {
		uint64_t inst_count = 0;
		std::map<uint32_t, uint64_t> inst_tab;

		auto &opcode_map = metrics.opcode_freq_hist.get_value_map();
		for (auto i = opcode_map.begin(); i != opcode_map.end(); ++i) {
			inst_tab[i->first] = *(i->second);
		}
		inst_count = metrics.opcode_freq_hist.get_total();

		stream << "Dynamic instruction frequencies, out of total " << std::dec << inst_count << std::endl;
		stream << "--------------------------------------------------" << std::endl;

		for (std::map<uint32_t, uint64_t>::iterator i = inst_tab.begin(); i != inst_tab.end(); ++i) {
			double x = ((double)i->second) / (double)inst_count * (double)100.0;

			if (i->second) {
				stream << GetDisasm()->GetInstrName(i->first) << "\t" << i->second << "\t" << x << "\n";
			}
		}
	}

	stream << std::endl;

	// Print out Instruction Summaries
	stream << "Instruction Execution Profile" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;
	stream << "                            Instructions   %Total" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;
	stream << " Translated instructions:    " << std::dec << std::setw(12) << metrics.native_inst_count.get_value() << "  " << std::fixed << std::setprecision(2) << std::setw(5) << std::right << (100.0 * metrics.native_inst_count.get_value() / instructions()) << std::endl;
	stream << " Interpreted instructions:   " << std::dec << std::setw(12) << metrics.interp_inst_count.get_value() << "  " << std::fixed << std::setprecision(2) << std::setw(5) << std::right << (100.0 * metrics.interp_inst_count.get_value() / instructions()) << std::endl;
	stream << " Total instructions:         " << std::dec << std::setw(12) << instructions() << "  100.00" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;

	stream << std::endl;

	GetEmulationModel().GetuArch().PrintStatistics(stream);

	stream << std::endl;

	// Print overall simulation times and simulated instructions
	double sim_total = exec_time.GetElapsedS();
	double compile_total = compile_time.GetElapsedS();
	double trc_total = trace_time.GetElapsedS();

	stream << "Simulation Time Statistics [Seconds]" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;
	stream << "        Simulation       Tracing         Total" << std::endl;
	stream << "-----------------------------------------------------" << std::endl;
	stream << " Time:  " << std::fixed << std::setprecision(2) << std::setw(10) << std::right << (sim_total - trc_total) << "    "
	       << std::fixed << std::setprecision(2) << std::setw(10) << std::right << trc_total << "    "
	       << std::fixed << std::setprecision(2) << std::setw(10) << std::right << sim_total << std::endl;
	stream << "-----------------------------------------------------" << std::endl;

	stream << "Time spent in native code = " << native_time.GetElapsedS() << " seconds" << std::endl;
	stream << "Time spent in interpreter = " << interp_time.GetElapsedS() << " seconds" << std::endl;
	stream << "Time spent in compiler    = " << compile_time.GetElapsedS() << " seconds" << std::endl;

	stream << std::endl;

	stream << "Translations executed    = " << metrics.txlns_invoked.get_value() << std::endl << std::endl;

	stream << "Interrupt Checks         = " << metrics.interrupt_checks.get_value() << std::endl;
	stream << "Interrupts Serviced      = " << metrics.interrupts_serviced.get_value() << std::endl;
	stream << "Syscalls Invoked         = " << metrics.syscalls_invoked.get_value() << std::endl;
	stream << "Device accesses          = " << metrics.device_accesses.get_value() << std::endl;
	stream << "Soft flushes             = " << metrics.soft_flushes.get_value() << std::endl;

	stream << "Chained target             = " << state.chained_target << std::endl;
	stream << "Chained fallthrough        = " << state.chained_fallthrough << std::endl;
	stream << "Chained failed             = " << state.chained_failed << std::endl;
	stream << "Chained can't              = " << state.chained_cantchain << std::endl;

	stream << "Big block              = " << metrics.too_large_block.get_value() << std::endl;


	stream << std::endl;


	uint32_t total = 0;
	stream << "Leave JIT Reasons" << std::endl;
	for (int i = 0; i < 7; i++) {
		stream << "  ";
		switch (i) {
			case 0:
				stream << "Normal            ";
				break;
			case 1:
				stream << "Require Interpret ";
				break;
			case 2:
				stream << "Unable to Chain   ";
				break;
			case 3:
				stream << "Interrupt Abort   ";
				break;
			case 4:
				stream << "Exception Abort   ";
				break;
			case 5:
				stream << "No Indirect Target";
				break;
			case 6:
				stream << "No Fixed Target   ";
				break;
			default:
				stream << "???";
				break;
		}
		stream << " = " << metrics.leave_jit[i].get_value() << std::endl;

		total += metrics.leave_jit[i].get_value();
	}

	stream << "  Total              = " << total << std::endl << std::endl;

	if (archsim::options::JitExtraCounters) {
		stream << "SMM Cache Statistics" << std::endl;

		stream << "  Read Cache" << std::endl;
		stream << "    A: " << metrics.smm_read_cache_accesses.get_value() << std::endl;
		stream << "    H: " << metrics.smm_read_cache_hits.get_value() << std::endl;
		stream << "    M: " << metrics.smm_read_cache_accesses.get_value() - metrics.smm_read_cache_hits.get_value() << std::endl;
		stream << "  Write Cache" << std::endl;
		stream << "    A: " << metrics.smm_write_cache_accesses.get_value() << std::endl;
		stream << "    H: " << metrics.smm_write_cache_hits.get_value() << std::endl;
		stream << "    M: " << metrics.smm_write_cache_accesses.get_value() - metrics.smm_write_cache_hits.get_value() << std::endl << std::endl;
	}

	if (mem_counter) {
		mem_counter->PrintStatistics(stream);
		stream << std::endl;
	}

	stream << "Simulation time = " << sim_total << " [Seconds]" << std::endl;
	stream << "Simulation rate = " << ((sim_total) ? (instructions() * 1.0e-6 / sim_total) : 0) << " [MIPS]" << std::endl;
}


void Processor::DumpRegisters()
{
	for (unsigned int bankIdx = 0; bankIdx < 1; bankIdx++) { // GetRegisterBankCount(); bankIdx++) {
		RegisterBankDescriptor& bank = GetRegisterBankDescriptor(bankIdx);

		fprintf(stderr, "BANK %d\n", bankIdx);
		for (unsigned int regIdx = 0; regIdx < bank.NumberOfRegistersInBank(); regIdx++) {
			fprintf(stderr, "  r%02d = %08x (%d)\n", regIdx, ((uint32_t *)bank.GetBankDataStart())[regIdx], ((uint32_t *)bank.GetBankDataStart())[regIdx]);
		}
	}

	for (unsigned int regIdx = 0; regIdx < GetRegisterCount(); regIdx++) {
		RegisterDescriptor& reg = GetRegisterDescriptor(regIdx);
		fprintf(stderr, "%s = %08x (%d)\n", reg.Name.c_str(), *((uint32_t *)reg.DataStart), *((uint32_t *)reg.DataStart));
	}
}
/* Memory access functions */

uint32_t Processor::mem_read_8(uint32_t addr, uint8_t& data)
{
	uint32_t zdata;
	uint32_t rval = GetMemoryModel().Read8_zx(addr, zdata);
	data = zdata;
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_read_8s(uint32_t addr, uint8_t& data)
{
	uint32_t zdata;
	uint32_t rval = GetMemoryModel().Read8_sx(addr, zdata);
	data = zdata;
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_read_8_user(uint32_t addr, uint8_t& data)
{
	uint32_t zdata;
	uint32_t rval = GetMemoryModel().Read8User(addr, zdata);
	data = zdata;
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_read_16(uint32_t addr, uint16_t& data)
{
	uint32_t zdata;
	uint32_t rval = GetMemoryModel().Read16_zx(addr, zdata);
	data = zdata;
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_read_16s(uint32_t addr, uint16_t& data)
{
	uint32_t zdata;
	uint32_t rval = GetMemoryModel().Read16_sx(addr, zdata);
	data = zdata;
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_read_32(uint32_t addr, uint32_t& data)
{
	uint32_t rval = GetMemoryModel().Read32(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_read_32_user(uint32_t addr, uint32_t& data)
{
	uint32_t rval = GetMemoryModel().Read32User(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Read(true, addr, data);
	return rval;
}

uint32_t Processor::mem_write_8(uint32_t addr, uint32_t data)
{
	uint32_t rval = GetMemoryModel().Write8(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Write(true, addr, data & 0xff);
	return rval;
}

uint32_t Processor::mem_write_8_user(uint32_t addr, uint32_t data)
{
	uint32_t rval = GetMemoryModel().Write8User(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Write(true, addr, data & 0xff);
	return rval;
}

uint32_t Processor::mem_write_16(uint32_t addr, uint32_t data)
{
	uint32_t rval = GetMemoryModel().Write16(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Write(true, addr, data & 0xffff);
	return rval;
}

uint32_t Processor::mem_write_32_user(uint32_t addr, uint32_t data)
{
	uint32_t rval = GetMemoryModel().Write32User(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Write(true, addr, data);
	return rval;
}

uint32_t Processor::mem_write_32(uint32_t addr, uint32_t data)
{
	uint32_t rval = GetMemoryModel().Write32(addr, data);
	if (!rval && UNLIKELY(IsTracingEnabled() && trace_mgr->IsPacketOpen())) trace_mgr->Trace_Mem_Write(true, addr, data);
	return rval;
}
