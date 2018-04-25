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

			archsim::util::Counter64 too_large_block;

			// Number of interpreted instructions
			//
			archsim::util::Counter64 interp_inst_count;

			// Number of natively executed instructions
			//
			archsim::util::Counter64 native_inst_count;
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

