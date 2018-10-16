/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ThreadMetrics.h
 * Author: harry
 *
 * Created on 25 April 2018, 14:16
 */

#ifndef THREADMETRICS_H
#define THREADMETRICS_H

#include "core/arch/ArchDescriptor.h"
#include "util/Counter.h"
#include "util/CounterTimer.h"
#include "util/Histogram.h"

#include <functional>
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
			};

			class ThreadMetricPrinter
			{
			public:
				void PrintStats(const ArchDescriptor &arch, const ThreadMetrics &metrics, std::ostream &str);
			};

			class HistogramPrinter
			{
			public:
				void PrintHistogram(const archsim::util::Histogram &hist, std::ostream &str, std::function<std::string(archsim::util::HistogramEntry::histogram_key_t)> key_formatter);
			};
		}
	}
}

#endif /* THREADMETRICS_H */

