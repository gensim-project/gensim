/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * signals.cpp
 *
 *  Created on: 16 Jan 2015
 *      Author: harry
 */

#include "signals.h"
#include "util/LogContext.h"

#include "system.h"
#include "abi/EmulationModel.h"
#include "abi/memory/MemoryModel.h"

#include <libtrace/TraceSource.h>

UseLogContext(LogInfrastructure);
DeclareChildLogContext(LogSignals, LogInfrastructure, "Signals");
DeclareChildLogContext(LogSegFault, LogSignals, "SegFault");

static std::vector<System *> sim_ctxs;

static void sigint_handler(siginfo_t *si, void *unused);
static void sigsegv_handler(siginfo_t *si, void *unused);
static void sigtrap_handler(siginfo_t *si, void *unused);
static void sigusr1_handler(siginfo_t *si, void *unused);
static void sigusr2_handler(siginfo_t *si, void *unused);

static void generic_signal_handler(int signo, siginfo_t *si, void *unused)
{
	for(auto sim_ctx : sim_ctxs) {
		if (sim_ctx->GetEmulationModel().HandleSignal(signo, si, unused)) {
			return;
		}
	}

	switch (signo) {
		case SIGINT:
			sigint_handler(si, unused);
			break;
		case SIGSEGV:
			sigsegv_handler(si, unused);
			break;
		case SIGTRAP:
			sigtrap_handler(si, unused);
			break;
		case SIGUSR1:
			sigusr1_handler(si, unused);
			break;
		case SIGUSR2:
			sigusr2_handler(si, unused);
			break;
		default:
			LC_ERROR(LogSignals) << "Unhandled captured signal " << signo;
			exit(-1);
	}
}

bool signals_capture()
{
	struct sigaction sa;

	bzero(&sa, sizeof(sa));
	sa.sa_sigaction = generic_signal_handler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
		LC_ERROR(LogSignals) << "Unable to capture SEGV signal";
		return false;
	}

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		LC_ERROR(LogSignals) << "Unable to capture INT signal";
		return false;
	}

	if (sigaction(SIGTRAP, &sa, NULL) == -1) {
		LC_ERROR(LogSignals) << "Unable to capture TRAP signal";
		return false;
	}

	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		LC_ERROR(LogSignals) << "Unable to capture USR1 signal";
		return false;
	}

	if (sigaction(SIGUSR2, &sa, NULL) == -1) {
		LC_ERROR(LogSignals) << "Unable to capture USR2 signal";
		return false;
	}

	return true;
}

void signals_register(System *sys)
{
	sim_ctxs.push_back(sys);
	LC_INFO(LogSignals) << "Registered system " << sys << " for signal capture";
}

static void sigusr1_handler(siginfo_t *si, void *unused)
{
	for(auto sim_ctx : sim_ctxs) {
//		fprintf(stderr, "CORE 0:\n");
//		fprintf(stderr, "  ICOUNT: %lu\n", sim_ctx->GetEmulationModel().GetBootCore()->instructions());
//		fprintf(stderr, "  PC:     %08x\n", sim_ctx->GetEmulationModel().GetBootCore()->read_pc());
		sim_ctx->PrintStatistics(std::cout);
	}
}

static void sigusr2_handler(siginfo_t *si, void *unused)
{
	for(auto sim_ctx : sim_ctxs) {
		UNIMPLEMENTED;
	}
}

static void sigint_handler(siginfo_t *si, void *unused)
{
	for(auto sim_ctx : sim_ctxs) {
		sim_ctx->HaltSimulation();
	}
}

static void sigtrap_handler(siginfo_t *si, void *unused)
{
	for(auto sim_ctx : sim_ctxs) {
		sim_ctx->HaltSimulation();
	}

	exit(-1);
}

static void sigsegv_handler(siginfo_t *si, void *unused)
{
	// Make sure we've got a simulation context at this point.
	if(sim_ctxs.size()) {
		//First, try and get a simulation context to handle the segfault
		for(auto sim_ctx : sim_ctxs) {
			if(sim_ctx->GetEmulationModel().GetMemoryModel().HandleSegFault((archsim::abi::memory::host_const_addr_t)si->si_addr)) return;
		}

		for(auto sim_ctx : sim_ctxs) {
			if(sim_ctx->HandleSegFault((uint64_t)si->si_addr)) return;
		}

		//None of our simulation contexts could handle the segfault, so abort.
		for(auto sim_ctx : sim_ctxs) {

			// Dump memory regions
			if (sim_ctx->GetEmulationModel().GetMemoryModel().GetMappingManager()) {
				sim_ctx->GetEmulationModel().GetMemoryModel().GetMappingManager()->DumpRegions();
			}
			
			// TODO: Figure out which thread caused the fault and print some diagnostics
		}
	}

	LC_ERROR(LogSegFault) << "Unhandled Memory Access Violation @ 0x" << std::hex << (unsigned long)si->si_addr;

	// Try and salvage tracing information
	for(auto sim_ctx : sim_ctxs) {
		sim_ctx->Destroy();
	}
	// Enough, now.
	exit(-1);
}

