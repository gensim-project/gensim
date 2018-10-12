/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ThreadMetrics.h
 * Author: harry
 *
 * Created on 25 April 2018, 14:16
 */

#ifndef THREADMETRICS_H
#define THREADMETRICS_H

#include "util/Counter.h"
#include "util/CounterTimer.h"
#include "util/Histogram.h"

#include <ostream>

namespace archsim
{
	namespace core
	{
		namespace thread
		{
			class ThreadMetrics
			{
			public:
				archsim::util::Counter64 InstructionCount;

				archsim::util::CounterTimer SelfRuntime;
				archsim::util::CounterTimer TotalRuntime;

				archsim::util::Histogram PCHistogram;
				archsim::util::Histogram OpcodeHistogram;
				archsim::util::Histogram InstructionIRHistogram;

				archsim::util::Counter64 ReadHits;
				archsim::util::Counter64 Reads;
				archsim::util::Counter64 WriteHits;
				archsim::util::Counter64 Writes;

				archsim::util::Counter64 JITInstructionCount;
				archsim::util::CounterTimer JITTime;

				archsim::util::Counter64 JITSuccessfulChains;
				archsim::util::Counter64 JITFailedChains;
			};

			class ThreadMetricPrinter
			{
			public:
				void PrintStats(const ThreadMetrics &metrics, std::ostream &str);
			};
		}
	}
}

#endif /* THREADMETRICS_H */

