//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
//  main -- This is the top-level source file for the command-line
//          instruction-set simulator
//
// =====================================================================

#include <climits>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <thread>

#include <limits.h>
#include <unistd.h>

#include "session.h"
#include "system.h"
#include "signals.h"


#include "util/ComponentManager.h"
#include "util/CommandLine.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"
#include "util/CounterTimer.h"

#include "arch/arm/ArmSystemEmulationModel.h"
#include "abi/devices/generic/block/MemoryCOWBlockDevice.h"

#include "abi/devices/generic/timing/TickSource.h"

#include "cmake-scm.h"

/**
 * Archsim copyright string
 */
#define ARCHSIM_COPYRIGHT QUOTEME(\n \
\t\tArchsim: The Edinburgh High Speed (EHS) Simulator\n \
\t\t\tUniversity of Edinburgh (c) 2017\n \
\t\t\t\tRevision : SCM_REV\n \
\t\t\t\tConfiguration: CONFIGSTRING\n\n)


#ifdef CONFIG_MEMTRACE
#include <mcheck.h>
#endif

UseLogContext(LogLogging);
UseLogContext(LogSignals);
UseLogContext(LogSystem);
UseLogContext(LogInfrastructure);

static int run_simple_simulation(archsim::Session& session)
{
#ifdef CONFIG_MEMTRACE
	mtrace();
#endif
	System *simsys = new System(session);

	if(archsim::options::InstructionTick) {
		simsys->SetTickSource(new archsim::abi::devices::timing::SubscriberTickSource(simsys->GetPubSub(), PubSubType::InstructionExecute, 1));
	} else {
		simsys->SetTickSource(new archsim::abi::devices::timing::MicrosecondTickSource(1000));
	}

	archsim::abi::devices::generic::block::BlockDevice *block_dev = nullptr;

	archsim::abi::devices::generic::block::FileBackedBlockDevice master_fbbd;
	if(archsim::options::BlockDeviceFile.IsSpecified()) {
		master_fbbd.Open(archsim::options::BlockDeviceFile.GetValue());

		if(archsim::options::CopyOnWrite) {
			archsim::abi::devices::generic::block::MemoryCOWBlockDevice *mem_cow = new archsim::abi::devices::generic::block::MemoryCOWBlockDevice();
			mem_cow->Open(master_fbbd);
			block_dev = mem_cow;
		} else {
			block_dev = &master_fbbd;
		}

		simsys->InstallBlockDevice("vda", block_dev);
	}

	// Configure the system object.
	if (!simsys->Initialise()) {
		LC_ERROR(LogInfrastructure) << "Unable to initialise simulation system";
		return -1;
	}

	// Store a pointer to the system object, and start the simulation.
	signals_register(simsys);

	simsys->RunSimulation();
	int rc = simsys->exit_code;

	if (archsim::options::Verbose) {
		simsys->PrintStatistics(std::cout);
	}

	// Destroy System after simulation to clean up resources
	simsys->Destroy();
	
	delete simsys;

	return rc;
}

static void run_system(System *sys)
{
	sys->RunSimulation();
}

/**
 * Application entry point
 * @param argc Number of command-line arguments (including program name)
 * @param argv Vector of command-line argument strings
 * @return Program exit code
 */
int main(int argc, char *argv[])
{
	archsim::util::CounterTimer init_timer;
	init_timer.Start();

	archsim::Session session;
	
	int rc;
	
	// First thing to do is parse the command-line.  This will print out any problems
	// with the input, so we can just exit with an error if parsing failed.
	archsim::util::CommandLineManager command_line;
	if (!command_line.Parse(argc, argv)) {
		rc = -1;
		goto out;
	}

	// Now, load modules. We still haven't initialised logging, so any messages
	// will go to the early log.
	session.GetModuleManager().LoadStandardModuleDirectory();


	// By default, we only show errors;
	archsim::options::LogLevel.SetValue(archsim::util::LogEvent::LL_ERROR);

	// Update the reporting level.  If verbose is specified, then we show information messages.
	if (archsim::options::Verbose && (archsim::options::LogLevel > archsim::util::LogEvent::LL_INFO)) {
		archsim::options::LogLevel.SetValue(archsim::util::LogEvent::LL_INFO);
	}

	// If debug is specified, then we show debug1 messages.
	if (archsim::options::Debug && !archsim::options::LogLevel.IsSpecified() && (archsim::options::LogLevel > archsim::util::LogEvent::LL_DEBUG1)) {
		archsim::options::LogLevel.SetValue(archsim::util::LogEvent::LL_DEBUG1);
	}

	// Initialise the logging subsystem.
	if (!archsim::util::LogManager::RootLogManager.Initialize()) {
		fprintf(stderr, "error: unable to initialise the logging subsystem\n");
		rc = -1;
		goto out;
	}

	// Parse the logging specifier file - if specified.
	if (archsim::options::LogSpec.IsSpecified()) {
		if(!archsim::util::LogManager::RootLogManager.LoadLogSpec(archsim::options::LogSpec)) {
			LC_ERROR(LogLogging) << "Could not parse logspec file";

			rc = -1;
			goto out;
		}
	}

	// Dump the log tree - if specified.
	if (archsim::options::LogTreeDump) {
		archsim::util::LogManager::RootLogManager.DumpGraph();
		rc = 0;
		goto out;
	}

	// Print out that verbose is enabled (we couldn't before as the logging subsystem wasn't ready)
	if (archsim::options::Verbose) {
		LC_INFO(LogInfrastructure) << "Verbose Reporting Enabled";
	}

	// Next, if help was specified, then print out usage information and simulation options,
	// then terminate nicely.
	if (archsim::options::Help) {
		command_line.PrintUsage();
		archsim::options::PrintOptions();

		rc = 0;
		goto out;
	}

	// Attempt to capture UNIX signals
	if (!signals_capture()) {
		LC_ERROR(LogSignals) << "Unable to capture signals";
		rc = -1;
		return rc;
	}

	// Print copyright header if quiet is not specified.
	if (!archsim::options::Quiet) {
		fprintf(stderr, ARCHSIM_COPYRIGHT);
	}

	init_timer.Stop();

	// Print statistics, if Verbose is specified.
	if (archsim::options::Verbose) {
		std::cout << "Initialisation Time: " << init_timer.GetElapsedS() << std::endl;
	}

	rc = run_simple_simulation(session);

out:
	return rc;
}
