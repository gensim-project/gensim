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

#include "gensim/gensim.h"

#include "util/ComponentManager.h"
#include "util/CommandLine.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"
#include "util/ModuleManager.h"
#include "util/PatchPoints.h"
#include "util/CounterTimer.h"

#include "arch/arm/ArmSystemEmulationModel.h"
#include "abi/devices/generic/block/MemoryCOWBlockDevice.h"

#include "abi/devices/generic/timing/TickSource.h"

#ifdef CONFIG_MEMTRACE
#include <mcheck.h>
#endif

UseLogContext(LogLogging);
UseLogContext(LogSignals);
UseLogContext(LogSystem);
DeclareLogContext(LogInfrastructure, "Infrastructure");

static int run_simple_simulation(archsim::Session& session)
{
#ifdef CONFIG_MEMTRACE
	mtrace();
#endif
	System simsys (session);
	if(!simsys.GetModuleManager().LoadModule(archsim::options::ProcessorModel.GetValue())) {
		LC_ERROR(LogSystem) << "Could not load processor module";
		return 1;
	}
	simsys.GetModuleManager().LoadStandardModuleDirectory();

	if(archsim::options::InstructionTick) {
		simsys.SetTickSource(new archsim::abi::devices::timing::SubscriberTickSource(simsys.GetPubSub(), PubSubType::InstructionExecute, 1));
	} else {
		simsys.SetTickSource(new archsim::abi::devices::timing::MicrosecondTickSource(1000));
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

		simsys.InstallBlockDevice("vda", block_dev);
	}

	// Configure the system object.
	if (!simsys.Initialise()) {
		LC_ERROR(LogInfrastructure) << "Unable to initialise simulation system";
		return -1;
	}

	// Store a pointer to the system object, and start the simulation.
	signals_register(&simsys);

	simsys.RunSimulation();
	int rc = simsys.exit_code;

	if (archsim::options::Verbose) {
		simsys.PrintStatistics(std::cout);
	}

	// Destroy System after simulation to clean up resources
	simsys.Destroy();

	return rc;
}

static void run_system(System *sys)
{
	sys->RunSimulation();
}

static int run_verify_simulation(archsim::Session& session)
{
	System &interp_sys = *new System(session);
	System &jit_sys = *new System(session);

	archsim::abi::devices::timing::TickSource *ts = new archsim::abi::devices::timing::CallbackTickSource(50000);

	interp_sys.SetTickSource(ts);
	jit_sys.SetTickSource(ts);

	System::InitVerify();

	interp_sys.block_device_file = archsim::options::BlockDeviceFile.GetValue() + ".interp";
	jit_sys.block_device_file = archsim::options::BlockDeviceFile.GetValue() + ".jit";

	interp_sys.EnableVerify();
	interp_sys.SetVerifyNext(&jit_sys);

	jit_sys.EnableVerify();
	jit_sys.SetVerifyNext(NULL);


	archsim::abi::devices::generic::block::FileBackedBlockDevice master_fbbd;
	master_fbbd.Open(archsim::options::BlockDeviceFile.GetValue());

	archsim::abi::devices::generic::block::MemoryCOWBlockDevice interp_block, jit_block;
	interp_block.Open(master_fbbd);
	jit_block.Open(master_fbbd);

	interp_sys.InstallBlockDevice("vda", &interp_block);
	jit_sys.InstallBlockDevice("vda", &jit_block);

	if(!interp_sys.Initialise() || !jit_sys.Initialise()) {
		LC_ERROR(LogInfrastructure) << "Unable to initialise simulation system";
		return -1;
	}

	interp_sys.SetSimulationMode(System::Interpreter);
	jit_sys.SetSimulationMode(System::JIT);

	signals_register(&interp_sys);
	signals_register(&jit_sys);

	std::thread interp_thread(run_system, &interp_sys);
	std::thread jit_thread(run_system, &jit_sys);

	interp_thread.join();
	jit_thread.join();

	interp_block.Close();
	jit_block.Close();
	master_fbbd.Close();

	interp_sys.PrintStatistics(std::cerr);
	jit_sys.PrintStatistics(std::cerr);

	interp_sys.Destroy();
	jit_sys.Destroy();

	return 0;
}


static int run_socket_verify_simulation(archsim::Session& session)
{
	System &sys = *new System(session);
	if(!sys.GetModuleManager().LoadModule(archsim::options::ProcessorModel.GetValue())) {
		LC_ERROR(LogSystem) << "Could not load processor module";
		return 1;
	}

	archsim::abi::devices::timing::TickSource *ts = new archsim::abi::devices::timing::CallbackTickSource(50000);

	archsim::abi::devices::generic::block::FileBackedBlockDevice master_fbbd;
	master_fbbd.Open(archsim::options::BlockDeviceFile.GetValue());

	archsim::abi::devices::generic::block::MemoryCOWBlockDevice cow;
	cow.Open(master_fbbd);

	sys.InstallBlockDevice("vda", &cow);

	sys.SetTickSource(ts);

	System::InitVerify();

	sys.block_device_file = archsim::options::BlockDeviceFile.GetValue();
	if(!sys.Initialise()) {
		LC_ERROR(LogInfrastructure) << "Unable to initialise simulation system";
		return -1;
	}

	sys.InitSocketVerify();

	sys.EnableVerify();

	signals_register(&sys);

	sys.RunSimulation();

	sys.Destroy();

	return 0;
}

static int run_verify_callback_simulation(archsim::Session& session)
{
	System &sys = *new System(session);

	archsim::abi::devices::timing::TickSource *ts = new archsim::abi::devices::timing::CallbackTickSource(50000);

	sys.SetTickSource(ts);

	System::InitVerify();

	sys.block_device_file = archsim::options::BlockDeviceFile.GetValue();
	if(!sys.Initialise()) {
		LC_ERROR(LogInfrastructure) << "Unable to initialise simulation system";
		return -1;
	}

	sys.EnableVerify();

	signals_register(&sys);

	sys.RunSimulation();

	sys.Destroy();

	return 0;
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
	archsim::util::CommandLineManager command_line;

	int rc;

	// By default, we only show errors;
	archsim::options::LogLevel.SetValue(archsim::util::LogEvent::LL_ERROR);

	// First thing to do is parse the command-line.  This will print out any problems
	// with the input, so we can just exit with an error if parsing failed.
	if (!command_line.Parse(argc, argv)) {
		rc = -1;
		goto out;
	}

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

	if(archsim::options::Verify) {
		rc = 0;
		if(archsim::options::VerifyMode == "process")
			run_verify_simulation(session);
		else if(archsim::options::VerifyMode == "socket")
			run_socket_verify_simulation(session);
		else if(archsim::options::VerifyMode == "callsonly")
			rc = run_verify_callback_simulation(session);
		else {
			LC_ERROR(LogSystem) << "Unknown verification mode";
		}

	} else rc = run_simple_simulation(session);

out:
	return rc;
}
