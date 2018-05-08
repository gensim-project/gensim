#include "util/LivePerformanceMeter.h"
#include "util/Counter.h"

#include <chrono>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

using namespace archsim::util;

PerformanceSource::PerformanceSource(std::string name) : name(name), last(0)
{

}


PerformanceSource::~PerformanceSource()
{

}

CounterPerformanceSource::CounterPerformanceSource(std::string name, const Counter64& counter) : PerformanceSource(name), counter(counter)
{

}

uint64_t CounterPerformanceSource::GetValue()
{
	return counter.get_value();
}

InstructionCountPerformanceSource::InstructionCountPerformanceSource(gensim::Processor& cpu) : PerformanceSource("Instruction Count"), cpu(cpu)
{

}

uint64_t InstructionCountPerformanceSource::GetValue()
{
	UNIMPLEMENTED;
//	return cpu.instructions();
}

LivePerformanceMeter::LivePerformanceMeter(gensim::Processor& cpu, std::vector<PerformanceSource *> sources, std::string filename, uint32_t period) : cpu(cpu), sources(sources), filename(filename), period(period), terminate(false)
{

}

void LivePerformanceMeter::run()
{
	FILE *f = fopen(filename.c_str(), "wt");

	uint64_t samples = 0;
	while (!terminate) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		usleep(period);
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

		fprintf(f, "%lu\t", samples++);
		for (auto source : sources) {
			uint64_t abs_value = source->GetValue();

			uint64_t delta_samples = source->GetDelta();
			uint64_t delta_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

			double rate = (((double)delta_samples * (double)1000000) / (double)delta_time);

			fprintf(f, "\"%s\"\t%lu\t%lu\t%lf\t", source->GetName().c_str(), abs_value, delta_samples, rate);
		}
		fprintf(f, "\n");

		fflush(f);
		samples++;
	}

	fclose(f);
}

void LivePerformanceMeter::stop()
{
	terminate = true;
	join();
}
