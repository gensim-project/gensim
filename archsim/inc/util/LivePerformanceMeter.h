/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LivePerformanceMeter.h
 * Author: s0457958
 *
 * Created on 20 August 2014, 10:21
 */

#ifndef LIVEPERFORMANCEMETER_H
#define	LIVEPERFORMANCEMETER_H

#include "define.h"
#include "concurrent/Thread.h"

#include <string>
#include <vector>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace util
	{
		class Counter64;

		class PerformanceSource
		{
		public:
			PerformanceSource(std::string name);
			virtual ~PerformanceSource();
			virtual uint64_t GetValue() = 0;

			inline uint64_t GetDelta()
			{
				uint64_t delta = GetValue() - last;
				last = GetValue();

				return delta;
			}

			inline std::string GetName() const
			{
				return name;
			}

		private:
			std::string name;
			uint64_t last;
		};

		class CounterPerformanceSource : public PerformanceSource
		{
		public:
			CounterPerformanceSource(std::string name, const Counter64& counter);

			uint64_t GetValue() override;

		private:
			const Counter64& counter;
		};

		class InstructionCountPerformanceSource : public PerformanceSource
		{
		public:
			InstructionCountPerformanceSource(gensim::Processor& cpu);

			uint64_t GetValue() override;

		private:
			gensim::Processor& cpu;
		};

		class LivePerformanceMeter : public archsim::concurrent::Thread
		{
		public:
			LivePerformanceMeter(gensim::Processor& cpu, std::vector<PerformanceSource *> sources, std::string filename, uint32_t period = 1e6);

			void run();
			void stop();

		private:
			gensim::Processor& cpu;
			std::vector<PerformanceSource *> sources;
			std::string filename;
			uint32_t period;

			volatile bool terminate;
		};
	}
}

#endif	/* LIVEPERFORMANCEMETER_H */

