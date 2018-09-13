/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/thread/ThreadMetrics.h"

using namespace archsim::core::thread;

void ThreadMetricPrinter::PrintStats(const ThreadMetrics& metrics, std::ostream &str)
{
	str << "Thread Metrics" << std::endl;
	str << "Instructions: " << metrics.InstructionCount.get_value() << std::endl;
	str << "Translated Instructions: " << metrics.JITInstructionCount.get_value() << std::endl;
	str << "(% JITTED): " << metrics.JITInstructionCount.get_value() / (float)(metrics.InstructionCount.get_value()) << std::endl;

	str << "Reads: " << metrics.Reads.get_value() << std::endl;
	str << "Read hits: " << metrics.ReadHits.get_value() << std::endl;
	str << "Writes: " << metrics.Writes.get_value() << std::endl;
	str << "Write hits: " << metrics.WriteHits.get_value() << std::endl;

	str << "Self Runtime: " << metrics.SelfRuntime.GetElapsedS() << " seconds" << std::endl;
	str << "JIT Runtime: " << metrics.JITTime.GetElapsedS() << " seconds" << std::endl;
	str << "Execution Rate: " << (metrics.InstructionCount.get_value() / 1000000.0) / metrics.SelfRuntime.GetElapsedS() << " MIPS" << std::endl;

	str << "JIT Rate: " << (metrics.JITInstructionCount.get_value() / 1000000.0) / metrics.JITTime.GetElapsedS() << " MIPS" << std::endl;
}
