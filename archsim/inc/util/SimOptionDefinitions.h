/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SimOptionDefinitions.h
 * Author: s0457958
 *
 * Created on 06 February 2014, 12:39
 */

#ifndef SIMOPTIONDEFINITIONS_H
#define	SIMOPTIONDEFINITIONS_H

#ifndef __MAY_INCLUDE_SD
# error "Do not include this file directly.  Include util/SimOptions.h instead."
#endif

DefineCategory(General, "General Settings");
DefineCategory(Profiling, "General Settings");
DefineCategory(Tracing, "General Settings");
DefineCategory(System, "General Settings");
DefineCategory(Simulation, "General Settings");
DefineCategory(JIT, "General Settings");
DefineCategory(LIR, "General Settings");
DefineCategory(PageArch, "General Settings");
DefineCategory(Platform, "General Settings");
DefineCategory(Special, "General Settings");
DefineCategory(GPU, "General Settings");

DefineFlag(General, Help, "Displays usage information for the running ArcSim.", false);
DefineSetting(General, ModuleDirectory, "Sets the directory containing module shared objects", STRINGIFY(ARCHSIM_MODULE_DIRECTORY));
DefineSetting(General, LogSpec, "Defines logging behaviour", "");
DefineIntSetting(General, LogLevel, "Defines minimum logging level", 0);
DefineSetting(General, LogTarget, "Specifies default logging target", "console");
DefineFlag(General, Debug, "Enables debugging output", false);
DefineFlag(General, LogTreeDump, "Dumps the logging context tree", false);

DefineFlag(General, Quiet, "Suppresses display of the ArcSim banner.", false);
DefineFlag(General, Verbose, "Enables verbose output reporting", false);
DefineFlag(General, Verify, "Enables JIT verification", false);
DefineSetting(General, VerifyMode, "Verification mode", "process");
DefineFlag(General, VerifyBlocks, "Verification should be done at block granularity", false);
DefineFlag(General, LivePerformance, "Enables live performance measurements", false);
DefineFlag(General, MemEventCounting, "Enables memory event counting", false);

DefineFlag(Profiling, Profile, "Enables profiling", false);
DefineFlag(Profiling, ProfilePcFreq, "Enables PC frequency profiling", false);
DefineFlag(Profiling, ProfileIrFreq, "Enables IR frequency profiling", false);

DefineFlag(Tracing, Trace, "Enables tracing output", false);
DefineFlag(Tracing, SimpleTrace, "Simplified tracing", false);
DefineFlag(Tracing, TraceSymbols, "Enables symbol resolution in tracing output", false);
DefineFlag(Tracing, SuppressTracing, "Suppress tracing output at system startup", false);
DefineSetting(Tracing, TraceMode, "Selects tracing output mode", "binary");
DefineSetting(Tracing, TraceFile, "Redirects tracing output to a file", "trace.out");
DefineSetting(Tracing, StdOutFile, "Redirects stdout to a file", "stdout");
DefineSetting(Tracing, StdErrFile, "Redirects stderr to a file", "stderr");
DefineSetting(Tracing, StdInFile, "Redirects a file to stdin", "stdin");

DefineSetting(System, ProcessorModel, "Selects the processor model to use", "");
DefineSetting(System, ProcessorName, "Selects the name of the processor implementation to use", "");
DefineSetting(System, EmulationModel, "Selects the emulation model to use", "");
DefineSetting(System, MemoryModel, "Selects the memory model to use", "");
DefineSetting(System, SystemMemoryModel, "Select the model to use for system memory", "base");
DefineFlag(System, LazyMemoryModelInvalidation, "Uses lazy invalidation for the memory model", false);
DefineFlag(System, MemoryCheckAlignment, "Enforce strict alignment on memory accesses", true);
DefineFlag(System, EnablePerfMap, "Enable Perf-compatible JIT map", false);

DefineIntSetting(System, TickScale, "Scale timer tick length to be x times longer", 1);

DefineSetting(System, Mode, "Selects the simulation mode to use", "interp");
DefineFlag(System, CacheModel, "Enables CPU cache modelling", false);

DefineSetting(System, TargetBinary, "Selects the target binary to execute", "target.x");
DefineSetting(System, TargetBinaryFormat, "Selects the target binary format", "elf");
DefineSetting(System, ZImageSymbolMap, "Symbol Map for a ZIMAGE binary", "");

DefineSetting(System, SCPText, "Text to pass into the simulator cache coprocessor", "");

DefineFlag(System, InstructionTick, "Use an instruction-based clock", false);

DefineListSetting(System, Breakpoints, "List of functions to insert breakpoints on", new std::list<std::string>());

DefineIntSetting(Simulation, SimulationPeriod, "Runs the simulation for the specified number of blocks", 10000);

DefineFlag(LIR, LirDisableChain, "Disables chaining in the LIR JIT", false);
DefineSetting(LIR, LirRegisterAllocator, "Selects the register allocator to use", "graph-colouring");

DefineFlag(System, UserPermitSignalHandling, "Allow or disable installation of signal handlers in user mode emulation", false);

DefineSetting(JIT, JitTranslationManager, "Selects the translation manager to user", "interp");
DefineSetting(JIT, JitEngine, "Selects the type of JIT engine to use", "llvm");
DefineIntSetting(JIT, JitThreads, "Chooses the number of threads to employ for JIT compilation", 1);
DefineSetting(JIT, JitInterruptScheme, "Selects the interrupt checking scheme to use when using the JIT", "backwards");
DefineFlag(JIT, JitDisableAA, "Disables custom alias-analysis in the JIT", false);
DefineIntSetting(JIT, JitHotspotThreshold, "Chooses the number of times a region must be profiled to become hot", 20);
DefineIntSetting(JIT, JitProfilingInterval, "Chooses the number of basic-blocks to execute before considering regions for compilation", 30000);
DefineIntSetting(JIT, TransCacheSize, "Sets the size of the translation cache", 8192);
DefineIntSetting(JIT, JitOptLevel, "Sets the optimisation level for translation", 3);
DefineFlag(JIT, JitDisableBranchOpt, "Disable branch optimisations", false);
DefineFlag(JIT, JitExtraCounters, "Enable extra JIT counters", false);
DefineFlag(JIT, JitDebugAA, "Produce alias-analysis debugging output", false);
DefineFlag(JIT, JitUseIJ, "Use the instruction JIT to perform non-native execution", false);
DefineFlag(JIT, JitChecksumPages, "Produce and check checksums of JITed code on generation and execution", false);
DefineFlag(JIT, JitSaveTranslations, "Keep JIT translations between simulation runs", false);
DefineFlag(JIT, JitLoadTranslations, "Keep JIT translations between simulation runs", false);

DefineFlag(JIT, AggressiveCodeInvalidation, "Invalidate all code on a cache flush, rather than just detected modifications", false);

DefineIntSetting(PageArch, PageArchByteBits, "???", 2);
DefineIntSetting(PageArch, PageArchOffsetBits, "???", 11);
DefineIntSetting(PageArch, PageArchIndexBits, "???", 12);

DefineIntSetting(System, GuestStackSize, "Chooses the default size for the guest stack", 0x1000000);

DefineListSetting(Platform, EnabledDevices, "List of devices to enable for this simulation", new std::list<std::string>());
DefineSetting(Platform, ScreenManagerType, "Type of screen manager to use", "Null");
DefineSetting(Platform, DeviceTreeFile, "Path to flattened device tree file", "");
DefineSetting(Platform, KernelArgs, "Arguments to pass into the linux kernel", "");
DefineSetting(Platform, RootFS, "Path to initrd rootfs", "");
DefineSetting(Platform, BlockDeviceFile, "Path to block device file", "");
DefineFlag(Platform, CopyOnWrite, "Copy on write block device", false);
DefineIntSetting(Platform, RootFSLocation, "Physical memory location to write initrd to", 0x800000);
DefineSetting(Platform, Bootloader, "Bootloader for ELF system emulation", "");
DefineFlag(Platform, SerialGrab, "Take control of the terminal for use as a serial port", false);


DefineFlag(System, ArmOabi, "Use the ARM OABI instead of EABI", false);

// Program Specific Options
DefineFlag(Special, Doom, "Enable DOOM mode", false);

// GPU Simulation Options
DefineFlag(GPU, GPU, "Run simulator with GPU", false);
DefineFlag(GPU, GPUMetrics, "Enable gathering statistics about GPU Shaders, use --gpu-metrics", false);
DefineFlag(GPU, GPUTracing, "Enable tracing of GPU Shaders, use --gpu-tracing", false);
DefineFlag(GPU, GPUDisasm, "Enable disassembly of GPU Shaders, use --gpu-disasm", false);
DefineFlag(GPU, GPULogging, "Enable logging of GPU execution, use --gpu-logging", false);
DefineFlag(GPU, GPUDumpMem, "Enable dumping of shader code as hexdump, use --gpu-dump-shader-hex", false);
DefineFlag(GPU, GPUSpecialLog, "Enable special logging of GPU execution, use --gpu-special-logging", false);
DefineFlag(GPU, GPUTiming, "Enable timing of GPU Shaders, use --gpu-timing", false);
DefineFlag(GPU, GPUReplayInstructions, "Enable dumping of instruction information with inputs to verify, use --gpu-replay.", false);
DefineIntSetting(GPU, GPUSimNumHostThreads, "Select number of host threads on which to map GPU shader program, default is set automatically based on core count. Use --gpu-num-host-threads.", 0xffffffff);
DefineIntSetting(GPU, GPUSimNumShaderCores, "Select number of shader cores to execute model with, Use --gpu-num-shader-cores.", 1);

#endif	/* SIMOPTIONDEFINITIONS_H */

