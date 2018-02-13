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
#include "gensim/gensim_processor.h"

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
		gensim::Processor *core = sim_ctx->GetEmulationModel().GetBootCore();

		if (!core->HasTraceManager()) {
			core->InitialiseTracing();
		}

		if (core->IsTracingEnabled())
			core->StopTracing();
		else
			core->StartTracing();

		if (core->GetEmulationModel().GetSystem().HaveTranslationManager()) {
			core->GetEmulationModel().GetSystem().GetTranslationManager().Invalidate();
		}
	}

	/*if (!sigusr2)
		return;

	ucontext_t *uc = (ucontext_t *)unused;
	fprintf(sigusr2, "%lx\n", uc->uc_mcontext.gregs[REG_RIP]);
	fflush(sigusr2);*/
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

		gensim::Processor *cpu = sim_ctx->GetEmulationModel().GetBootCore();
		LC_ERROR(LogInfrastructure) << "Encountered trap at 0x" << std::hex << cpu->read_pc() << " (" << cpu->GetMemoryModel().Peek32Unsafe(cpu->read_pc()) << ") mode " << (uint32_t)cpu->get_cpu_mode();

		if (cpu != NULL && cpu->HasTraceManager() && cpu->IsTracingEnabled()) {
			cpu->GetTraceManager()->Trace_End_Insn();
			cpu->GetTraceManager()->Terminate();
		}
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
			if (sim_ctx->GetEmulationModel().GetMemoryModel().GetMappingManager())
				sim_ctx->GetEmulationModel().GetMemoryModel().GetMappingManager()->DumpRegions();

			gensim::Processor *cpu = sim_ctx->GetEmulationModel().GetBootCore();

			// Make sure we've got a boot CPU.  This isn't necessarily the right thing to do
			// here if we support multiple cores - as if the segfault was in guest code, we
			// don't know which emulated core caused the fault.
			if (cpu != NULL) {
				LC_ERROR(LogSegFault) << "Memory Access Violation @ 0x" << std::hex << (unsigned long)si->si_addr;

				// Print out some useful information.
				uint32_t pc = cpu->read_pc();
				LC_ERROR(LogSegFault) << "Current PC:       0x" << std::hex << pc;
				LC_ERROR(LogSegFault) << "Physical Address: 0x" << std::hex << (unsigned long)si->si_addr;

				archsim::abi::memory::guest_addr_t resolved_address;

				// Try and get the memory model to resolve the faulting address to
				// a guest virtual address.
				if (sim_ctx->GetEmulationModel().GetMemoryModel().ResolveGuestAddress(si->si_addr, resolved_address)) {
					LC_ERROR(LogSegFault) << "Virtual Address:  0x" << std::hex << (unsigned long)resolved_address;
				} else {
					LC_ERROR(LogSegFault) << "Faulting address did not resolve to a virtual address";
				}
			}
		}
	}

	LC_ERROR(LogSegFault) << "Unhandled Memory Access Violation @ 0x" << std::hex << (unsigned long)si->si_addr;

	// Try and salvage tracing information
	for(auto sim_ctx : sim_ctxs) {
		gensim::Processor *cpu = sim_ctx->GetEmulationModel().GetBootCore();
		if (cpu != NULL && cpu->HasTraceManager() && cpu->IsTracingEnabled()) {
			cpu->GetTraceManager()->Trace_End_Insn();
			cpu->GetTraceManager()->Terminate();
		}
	}
	// Enough, now.
	exit(-1);
}

