/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

#include <ostream>

namespace archsim {
	namespace core {
		namespace thread {
			class ThreadMetrics {
			public:
				archsim::util::Counter64 InstructionCount;
				
				archsim::util::CounterTimer SelfRuntime;
				archsim::util::CounterTimer TotalRuntime;
			};
			
			class ThreadMetricPrinter {
			public:
				void PrintStats(const ThreadMetrics &metrics, std::ostream &str);
			};
		}
	}
}

#endif /* THREADMETRICS_H */

