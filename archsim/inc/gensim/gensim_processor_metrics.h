/*
 * File:   gensim_processor_metrics.h
 * Author: s0457958
 *
 * Created on 18 February 2014, 17:09
 */

#ifndef GENSIM_PROCESSOR_METRICS_H
#define	GENSIM_PROCESSOR_METRICS_H

#include "util/Histogram.h"
#include "util/MultiHistogram.h"
#include "util/Counter.h"
#include "util/CounterTimer.h"

namespace gensim
{
	namespace processor
	{
		class ProcessorMetrics
		{
		public:
			// Frequency of each OpCode
			//
			archsim::util::Histogram opcode_freq_hist;
			// Instruction Frequencies
			//
			archsim::util::Histogram pc_freq_hist;
			// 'Function call' frequency (Call count)
			//
			archsim::util::Histogram call_freq_hist;
			// Instruction IR frequencies
			//
			archsim::util::Histogram inst_ir_freq_hist;
			// Long Immediate Frequencies
			//
			archsim::util::Histogram limm_freq_hist;
			// Number of times an instruction is not executed in a delay slot (delay killed)
			//
			archsim::util::Histogram dkilled_freq_hist;
			// Number of times an instruction was not executed (killed)
			//
			archsim::util::Histogram killed_freq_hist;
			// Records the frequecies of all targets for each call site
			//
			archsim::util::MultiHistogram call_graph_multihist;
			// Cycles per instruction histogram
			//
			archsim::util::Histogram inst_cycles_hist;
			// Per Opcode latency distribution
			//
			archsim::util::MultiHistogram opcode_latency_multihist;
			// Frequency of ICACHE/DCACHE misses per PC
			//
			archsim::util::Histogram icache_miss_freq_hist;
			archsim::util::Histogram dcache_miss_freq_hist;
			// Sum of ICACHE/DCACHE miss cycles per PC
			//
			archsim::util::Histogram icache_miss_cycles_hist;
			archsim::util::Histogram dcache_miss_cycles_hist;
			// Number of delay slot instructions
			//
			archsim::util::Counter64 dslot_inst_count;
			archsim::util::Counter64 too_large_block;
			// Flag stall count
			//
			archsim::util::Counter64 flag_stall_count;
			// Number of interpreted instructions
			//
			archsim::util::Counter64 interp_inst_count;
			// Number of natively executed instructions
			//
			archsim::util::Counter64 native_inst_count;
			// Current cycle count
			//
			archsim::util::Counter64 cycle_count;
			// Interrupt check count
			//
			archsim::util::Counter64 interrupt_checks;
			// Interrupts serviced count
			//
			archsim::util::Counter64 interrupts_serviced;
			// Syscalls Invoked count
			//
			archsim::util::Counter64 syscalls_invoked;
			// Translations Invoked count
			//
			archsim::util::Counter64 txlns_invoked;

			archsim::util::Counter64 smm_read_cache_accesses;
			archsim::util::Counter64 smm_read_cache_hits;
			archsim::util::Counter64 smm_write_cache_accesses;
			archsim::util::Counter64 smm_write_cache_hits;

			archsim::util::Counter64 soft_flushes;

			archsim::util::Counter64 leave_jit[8];

			archsim::util::Counter64 invalidations;
			archsim::util::Counter64 device_accesses;
		};
	}
}

#endif	/* GENSIM_PROCESSOR_METRICS_H */

