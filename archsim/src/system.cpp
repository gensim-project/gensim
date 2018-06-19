/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * system.cpp
 */
#include "system.h"

#include "abi/EmulationModel.h"
#include "abi/memory/MemoryCounterEventHandler.h"
#include "abi/devices/generic/timing/TickSource.h"

#include "core/thread/ThreadInstance.h"
#include "core/thread/ThreadMetrics.h"

#include "translate/TranslationManager.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"
#include "util/LivePerformanceMeter.h"

#include "uarch/uArch.h"

#include <iostream>
#include <libtrace/TraceSink.h>

DeclareLogContext(LogSystem, "System");
DeclareLogContext(LogInfrastructure, "Infrastructure");

System::System(archsim::Session& session) :
	session(session),
	exit_code(EXIT_SUCCESS),
	uarch(NULL),
	emulation_model(NULL),
	_halted(false),
	_tick_source(NULL),
	code_region_tracker_(pubsubctx)
{
	max_fd = 0;
	OpenFD(STDIN_FILENO);
	OpenFD(STDOUT_FILENO);
	OpenFD(STDERR_FILENO);
}

System::~System()
{
}

/**
 * Initialises the simulation system
 * @return Returns true if the simulation system was successfully initialised, false otherwise.
 */
bool System::Initialise()
{
	LC_DEBUG1(LogSystem) << "Initialising System";
	
	if(archsim::options::Trace) {
		libtrace::TraceSink *sink = nullptr;
		
		if(archsim::options::TraceMode == "binary") {
			if(!archsim::options::TraceFile.IsSpecified()) {
				UNIMPLEMENTED;
			}
			FILE *f = fopen(archsim::options::TraceFile.GetValue().c_str(), "w");
			if(f == nullptr) {
				UNIMPLEMENTED;
			}
			
			sink = new libtrace::BinaryFileTraceSink(f);
			
		} else {
			UNIMPLEMENTED;
		}
		
		GetECM().SetTraceSink(sink);
	}

	// Create and initialise the emulation model
	if (!GetComponentInstance(archsim::options::EmulationModel, emulation_model)) {

		LC_ERROR(LogSystem) << "Unable to create emulation model '" << archsim::options::EmulationModel.GetValue() << "'";
		LC_ERROR(LogSystem) << GetRegisteredComponents(emulation_model);
		return false;
	}

	uarch = new archsim::uarch::uArch();

	if (!uarch->Initialise()) {
		delete uarch;

		LC_ERROR(LogSystem) << "Unable to initialise uArch model";
		return false;
	}

	if (!emulation_model->Initialise(*this, *uarch)) {
		if(uarch != nullptr) {
			uarch->Destroy();
			delete uarch;
		}

		LC_ERROR(LogSystem) << "Unable to initialise emulation model '" << archsim::options::EmulationModel.GetValue() << "'";
		return false;
	}

	return true;
}

void System::Destroy()
{
	LC_DEBUG1(LogSystem) << "Destroying system";

	if(GetECM().GetTraceSink()) {
		GetECM().GetTraceSink()->Flush();
	}
	
	emulation_model->Destroy();
	delete emulation_model;

	uarch->Destroy();
	delete uarch;
}

void System::PrintStatistics(std::ostream& stream)
{
	stream << std::endl;

	stream << "Event Statistics" << std::endl;
	pubsubctx.PrintStatistics(stream);

	stream << "Thread Statistics" << std::endl;
	archsim::core::thread::ThreadMetricPrinter printer;
	for(auto context : GetECM()) {
		for(auto thread : context->GetThreads()) {
			printer.PrintStats(thread->GetMetrics(), stream);
		}
	}
	
	stream << "Simulation Statistics" << std::endl;

	// Print Emulation Model statistics
	emulation_model->PrintStatistics(stream);

	stream << std::endl;

	stream << std::endl;
}

bool System::RunSimulation()
{
	if (!emulation_model->PrepareBoot(*this)) return false;
	GetECM().Start();
	GetECM().Join();

	return true;
}

void System::HaltSimulation()
{
	_halted = true;
	emulation_model->HaltCores();

}

bool System::HandleSegFault(uint64_t addr)
{
	if (segfault_handlers.size() == 0)
		return false;

	segfault_handler_map_t::const_iterator iter = segfault_handlers.upper_bound(addr);
	iter--;

	if (addr >= iter->first && (addr < iter->first + iter->second.size)) {
		segfault_data data;
		data.addr = addr;

		bool falafel = iter->second.handler(iter->second.ctx, data);
		return falafel;
	} else {
		return false;
	}
}
