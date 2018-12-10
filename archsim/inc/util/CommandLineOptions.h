/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   CommandLineOptions.h
 * Author: s0457958
 *
 * Created on 06 February 2014, 16:11
 */

#ifndef COMMANDLINEOPTIONS_H
#define	COMMANDLINEOPTIONS_H

#include "SimOptionDefinitions.h"

DefineLongRequiredArgument(std::string, ModuleDirectory, "modules");

DefineFlag(Help, 'h', "help");
DefineFlag(Quiet, 'q', "quiet");
DefineFlag(Debug, 'd', "debug");
DefineFlag(Verbose, 'v', "verbose");
DefineLongFlag(Profile, "profile");
DefineLongFlag(ProfilePcFreq, "profile-pc");
DefineLongFlag(ProfileIrFreq, "profile-ir");

DefineLongFlag(EnablePerfMap, "enable-perf-map");

DefineRequiredArgument(uint32_t, LogLevel, 'g', "log-level");
DefineLongRequiredArgument(std::string, LogSpec, "logspec");
DefineLongRequiredArgument(std::string, LogTarget, "log-target");
DefineLongFlag(LogTreeDump, "dump-log-tree");

DefineRequiredArgument(std::string, ProcessorModel, 'p', "processor");
DefineRequiredArgument(std::string, ProcessorName, 's', "processor-name");
DefineRequiredArgument(std::string, MemoryModel, 'l', "memory");
DefineRequiredArgument(std::string, EmulationModel, 'm', "emulation");
DefineRequiredArgument(std::list<std::string> *, EnabledDevices, 'D', "enabled-devices");

DefineLongFlag(LivePerformance, "live-perf");
DefineLongFlag(MemEventCounting, "count-mem-events");
DefineLongFlag(CacheModel, "cache-model");

DefineFlag(ArmOabi, 'b', "oabi");

DefineLongRequiredArgument(uint32_t, StackFaffle, "stack-faffle");
DefineRequiredArgument(std::string, TargetBinary, 'e', "target");
DefineLongRequiredArgument(std::string, ZImageSymbolMap, "symbol-map");

DefineFlag(Trace, 't', "trace");
DefineLongRequiredArgument(uint64_t, TraceSkip, "trace-skip");
DefineFlag(SimpleTrace, 'T', "simple-trace");
DefineLongFlag(TraceSymbols, "trace-symbols");
DefineRequiredArgument(std::string, TraceMode, 'M', "trace-mode");
DefineOptionalArgument(std::string, TraceFile, 'U', "trace-file");
DefineLongRequiredArgument(std::list<std::string> *, Breakpoints, "breakpoints");
DefineLongFlag(SuppressTracing, "suppress-tracing");

DefineLongFlag(InstructionTick, "instruction-tick");

DefineLongRequiredArgument(uint32_t, TickScale, "tick-scale");

DefineLongRequiredArgument(std::string, JitInterruptScheme, "icp");
DefineLongRequiredArgument(std::string, JitEngine, "engine");
DefineLongRequiredArgument(uint32_t, JitThreads, "fast-num-threads");
DefineLongRequiredArgument(std::string, Mode, "mode");
DefineLongFlag(JitDisableAA, "no-aa");
DefineLongFlag(JitDebugAA, "debug-aa");
DefineLongFlag(JitUseIJ, "jit-use-ij");
DefineLongRequiredArgument(uint32_t, JitHotspotThreshold, "hotspot-threshold");
DefineLongRequiredArgument(uint32_t, JitProfilingInterval, "profiling-interval");
DefineRequiredArgument(uint32_t, JitOptLevel, 'O', "opt-level");

DefineLongFlag(JitExtraCounters, "extra-counters");

DefineLongFlag(LirDisableChain, "lir-disable-chain");
DefineLongRequiredArgument(std::string, LirRegisterAllocator, "lir-register-allocator");

DefineLongRequiredArgument(uint32_t, GuestStackSize, "guest-stack");

DefineLongRequiredArgument(std::string, TargetBinaryFormat, "binary-format");
DefineLongRequiredArgument(std::string, DeviceTreeFile, "device-tree");
DefineLongRequiredArgument(std::string, RootFS, "root-fs");
DefineLongRequiredArgument(std::string, BlockDeviceFile, "bdev-file");
DefineLongFlag(CopyOnWrite, "copy-on-write");
DefineLongRequiredArgument(std::string, KernelArgs, "kernel-args");
DefineLongRequiredArgument(std::string, Bootloader, "bootloader");
DefineLongRequiredArgument(std::string, SystemMemoryModel, "sys-model");
DefineLongFlag(LazyMemoryModelInvalidation, "lazy-mem-inv");
DefineLongRequiredArgument(std::string, ScreenManagerType, "screen");
DefineLongFlag(SerialGrab, "grab-serial");

DefineLongRequiredArgument(std::string, JitTranslationManager, "txln-mgr");
DefineLongFlag(JitDisableBranchOpt, "disable-branch-opt");

DefineLongFlag(JitLoadTranslations, "jit-load-txlns");
DefineLongFlag(JitSaveTranslations, "jit-save-txlns");

// Special Options
DefineLongFlag(Doom, "doom");

DefineLongFlag(Verify, "verify");
DefineLongFlag(VerifyBlocks, "verify-blocks");
DefineLongRequiredArgument(std::string, SCPText, "scp-text");

DefineLongRequiredArgument(std::string, VerifyMode, "verify-mode");

DefineLongFlag(AggressiveCodeInvalidation, "aggressive-code-invalidation");

// GPU Simulation Options
DefineLongRequiredArgument(uint32_t, GPUSimNumHostThreads, "gpu-num-host-threads");
DefineLongRequiredArgument(uint32_t, GPUSimNumShaderCores, "gpu-num-shader-cores");
DefineLongFlag(GPU, "gpu");
DefineLongFlag(GPUMetrics, "gpu-metrics");
DefineLongFlag(GPUTracing, "gpu-tracing");
DefineLongFlag(GPUDisasm, "gpu-disasm");
DefineLongFlag(GPULogging, "gpu-logging");
DefineLongFlag(GPUDumpMem, "gpu-dump-shader-hex");
DefineLongFlag(GPUSpecialLog, "gpu-special-logging");
DefineLongFlag(GPUTiming, "gpu-timing");
DefineLongFlag(GPUReplayInstructions, "gpu-replay");
#endif	/* COMMANDLINEOPTIONS_H */
