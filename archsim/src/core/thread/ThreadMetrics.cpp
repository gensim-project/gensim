/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/thread/ThreadMetrics.h"

using namespace archsim::core::thread;

void ThreadMetricPrinter::PrintStats(const ThreadMetrics& metrics, std::ostream &str)
{
	str << "Thread Metrics" << std::endl;
	str << "Instructions: " << metrics.InstructionCount.get_value() << std::endl;
	
	str << "Self Runtime: " << metrics.SelfRuntime.GetElapsedS() << " seconds" << std::endl;
	str << "Execution Rate: " << (metrics.InstructionCount.get_value() / 1000000.0) / metrics.SelfRuntime.GetElapsedS() << " MIPS" << std::endl;
}
