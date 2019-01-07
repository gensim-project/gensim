/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/thread/ThreadMetrics.h"
#include "gensim/gensim_disasm.h"
#include "util/SimOptions.h"

#include <fstream>
#include <sstream>

using namespace archsim::core::thread;

void ThreadMetricPrinter::PrintStats(const ArchDescriptor &arch, const ThreadMetrics& metrics, std::ostream &str)
{
	HistogramPrinter hp;

	str << "Thread Metrics" << std::endl;

	// TODO: Improve profiling to be per-isa
	if(archsim::options::Profile) {
		str << "Instruction Profile" << std::endl;

		auto disasm = arch.GetISA(0).GetDisasm();
		if(disasm != nullptr) {
			hp.PrintHistogram(metrics.OpcodeHistogram, str, [disasm](uint32_t i) {
				return disasm->GetInstrName(i);
			});
		} else {
			str << "(No instruction disassembly available)" << std::endl;
			hp.PrintHistogram(metrics.OpcodeHistogram, str, [disasm](uint32_t i) {
				return std::to_string(i);
			});
		}
	}
	if(archsim::options::ProfileIrFreq) {
		std::ofstream ir_str ("ir_freq.out");
		hp.PrintHistogram(metrics.InstructionIRHistogram, ir_str, [](uint32_t i) {
			std::stringstream str;
			str << std::hex << i;
			return str.str();
		});
	}
	if(archsim::options::ProfilePcFreq) {
		std::ofstream pc_str("pc_freq.out");
		hp.PrintHistogram(metrics.PCHistogram, pc_str, [](uint32_t i) {
			std::stringstream str;
			str << std::hex << i;
			return str.str();
		});
	}

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
	str << "Interpreter Rate: " << ((metrics.InstructionCount.get_value() - metrics.JITInstructionCount.get_value()) / 1000000.0) / (metrics.SelfRuntime.GetElapsedS() - metrics.JITTime.GetElapsedS()) << " MIPS" << std::endl;

	str << "Successful chains: " << metrics.JITSuccessfulChains.get_value() << std::endl;
	str << "Failed chains: " << metrics.JITFailedChains.get_value() << std::endl;
}

void HistogramPrinter::PrintHistogram(const archsim::util::Histogram& hist, std::ostream& str, std::function<std::string(archsim::util::HistogramEntry::histogram_key_t) > key_formatter)
{
	for(auto i : hist.get_value_map()) {
		str << key_formatter(i.first) << "\t" << *i.second << std::endl;
	}
}
