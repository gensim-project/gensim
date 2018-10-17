/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "system.h"
#include "abi/UserEmulationModel.h"
#include "abi/loader/BinaryLoader.h"
#include "abi/user/SyscallHandler.h"
#include "util/SimOptions.h"
#include "util/CommandLine.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "core/MemoryInterface.h"
#include "core/execution/ExecutionEngineFactory.h"

#include <sys/param.h>

extern char **environ;

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogEmulationModelUser, LogEmulationModel, "User");

using namespace archsim::abi;
using archsim::Address;

UserEmulationModel::UserEmulationModel(const user::arch_descriptor_t &arch, bool is_64bit_binary, const AuxVectorEntries &auxvs) : syscall_handler_(user::SyscallHandlerProvider::Singleton().Get(arch)), is_64bit_(is_64bit_binary), auxvs_(auxvs) { }

UserEmulationModel::~UserEmulationModel() { }

void UserEmulationModel::PrintStatistics(std::ostream& stream)
{
//	cpu->PrintStatistics(stream);

}

bool UserEmulationModel::InvokeSignal(int signum, uint32_t next_pc, SignalData* data)
{
	LC_DEBUG1(LogEmulationModelUser) << "Invoking Signal " << signum << ", Next PC = " << std::hex << next_pc << ", Target PC = " << std::hex << data->pc;
	return false;
}

bool UserEmulationModel::AssertSignal(int signal, SignalData* data)
{
	UNIMPLEMENTED;
//	cpu->assert_signal(signal);
	return true;
}

bool UserEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!EmulationModel::Initialise(system, uarch))
		return false;

	auto module = GetSystem().GetModuleManager().GetModule(archsim::options::ProcessorName);

	execution_engine_ = archsim::core::execution::ExecutionEngineFactory::GetSingleton().Get(module, "");
	GetSystem().GetECM().AddEngine(execution_engine_);

	auto main_thread = CreateThread(nullptr);
	StartThread(main_thread);

	return true;
}

archsim::core::thread::ThreadInstance* UserEmulationModel::CreateThread(archsim::core::thread::ThreadInstance* cloned_thread)
{
	auto module = GetSystem().GetModuleManager().GetModule(archsim::options::ProcessorName);
	auto archentry = module->GetEntry<archsim::module::ModuleArchDescriptorEntry>("ArchDescriptor");
	if(archentry == nullptr) {
		return nullptr;
	}
	auto arch = archentry->Get();

	auto thread = new archsim::core::thread::ThreadInstance(GetSystem().GetPubSub(), *arch, *this);
	int idx = 0;
	for(auto i : thread->GetMemoryInterfaces()) {
		i->Connect(*new archsim::CachedLegacyMemoryInterface(idx, GetMemoryModel(), thread));
		i->ConnectTranslationProvider(*new archsim::IdentityTranslationProvider());
		idx++;
	}

	if(cloned_thread) {
		memcpy(thread->GetRegisterFile(), cloned_thread->GetRegisterFile(), thread->GetArch().GetRegisterFileDescriptor().GetSize());
	}

	threads_.push_back(thread);

	return thread;
}

void UserEmulationModel::StartThread(archsim::core::thread::ThreadInstance* thread)
{
	execution_engine_->AttachThread(thread);
}

unsigned int UserEmulationModel::GetThreadID(const archsim::core::thread::ThreadInstance* thread) const
{
	unsigned int id = 0;
	for(auto i : threads_) {
		id++;
		if(thread == i) {
			return id;
		}
	}

	UNEXPECTED;
}

void UserEmulationModel::Destroy()
{
//	UNIMPLEMENTED;
}

archsim::core::thread::ThreadInstance* UserEmulationModel::GetMainThread()
{
	return threads_.at(0);
}

void UserEmulationModel::ResetCores()
{
	UNIMPLEMENTED;
}

void UserEmulationModel::HaltCores()
{
	GetMainThread()->SendMessage(core::thread::ThreadMessage::Halt);
}

bool UserEmulationModel::InitialiseProgramArguments()
{
	global_argc = archsim::util::CommandLineManager::guest_argc;
	global_argv = archsim::util::CommandLineManager::guest_argv;

	global_envc = 0;
	char **p = environ;

	while (*p++) global_envc++;

	return true;
}

bool UserEmulationModel::PrepareStack(System &system, Address elf_phdr_location, uint32_t elf_phnum, uint32_t elf_phentsize)
{
	_stack_size = archsim::options::GuestStackSize;
	_initial_stack_pointer = GetMemoryModel().GetMappingManager()->MapAnonymousRegion(_stack_size, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite)) + _stack_size;

//	if(Is64BitBinary()) {
//		_initial_stack_pointer = Address(0x800000000000);
//	} else {
//		_initial_stack_pointer = Address(0xc0000000);
//	}
//
//
//	if(_stack_size & ((1 << 12)-1)) {
//		LC_ERROR(LogEmulationModelUser) << "[USER-EMULATION] Guest stack size must be a multiple of host page size";
//		return false;
//	}
//
//	GetMemoryModel().GetMappingManager()->MapRegion(_initial_stack_pointer - _stack_size, _stack_size, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[stack]");

	std::vector<Address> envp_ptrs (global_envc);
	std::vector<Address> argv_ptrs (global_argc + 1);

	int argc = global_argc + 1;
	int envc = global_envc;

	Address sp = _initial_stack_pointer;
	// bias the stack pointer by the given stack faffle
	sp -= archsim::options::StackFaffle.GetValue();

//	printf("Start (%08x)\n", sp);

#define PUSH(_val)   do { if(Is64BitBinary()) { sp -= 8; GetMemoryModel().Write64(sp, _val); } else { sp -= 4; GetMemoryModel().Write32(sp, _val);  } } while (0)
#define PUSHB(_val)   do { sp -= 1; GetMemoryModel().Write8(sp, _val); } while (0)
#define PUSHSTR(_str)  do { sp -= (strlen(_str) + 1); GetMemoryModel().WriteString(sp, _str); } while (0)
#define ALIGN_STACK(v) do { sp -= ((uint64_t)sp.Get() & (v - 1)); } while (0)
#define PUSH_AUX_ENT(_id, _value) do { PUSH(_value); PUSH(_id); } while (0)

	PUSHSTR(auxvs_.PlatformName.c_str());
	Address platform_ptr = sp;

	PUSH(0);
	char the_realpath[MAXPATHLEN];
	realpath(archsim::options::TargetBinary.GetValue().c_str(), the_realpath);
	PUSHSTR(the_realpath); // global_argv[0]);
	argv_ptrs[0] = sp;

	for (int i = global_envc-1; i >= 0; i--) {
		if(environ[i][0] == '_') {
			std::string envstr = "_=" + archsim::options::TargetBinary.GetValue();
			PUSHSTR(envstr.c_str());
		} else {
			PUSHSTR(environ[i]);
		}
		envp_ptrs[i] = sp;
	}

	for(int i = 0; i < global_argc; ++i) {
		PUSHSTR(global_argv[i]);
		argv_ptrs[i+1] = sp;
	}

	for(int i = 0; i < 16; ++i) {
		PUSHB(0x1f);
	}
	Address random_ptr = sp;

	ALIGN_STACK(16);

	uint32_t entry_size = Is64BitBinary() ? 8 : 4;

	std::vector<std::pair<uint64_t, uint64_t>> auxv_entries;
	int elf_info_idx = 0;
#define NEW_AUXV_ENTRY(a, b) do { auxv_entries.push_back({a, b}); elf_info_idx += 2; } while(0)
	NEW_AUXV_ENTRY(AT_SYSINFO_EHDR, 0x7fff00000000);
	NEW_AUXV_ENTRY(AT_HWCAP, auxvs_.HWCAP);
	NEW_AUXV_ENTRY(AT_PAGESZ, 4096);
	NEW_AUXV_ENTRY(AT_PHDR, elf_phdr_location.Get());
	NEW_AUXV_ENTRY(AT_PHENT, elf_phentsize);
	NEW_AUXV_ENTRY(AT_PHNUM, elf_phnum);
//	NEW_AUXV_ENTRY(AT_BASE, 0);
	NEW_AUXV_ENTRY(AT_CLKTCK, 0x64);
//	NEW_AUXV_ENTRY(AT_FLAGS, 0x0);
	NEW_AUXV_ENTRY(AT_ENTRY, _initial_entry_point.Get());
	NEW_AUXV_ENTRY(AT_UID, 1000);
	NEW_AUXV_ENTRY(AT_EUID, 1000);
	NEW_AUXV_ENTRY(AT_GID, 1000);
	NEW_AUXV_ENTRY(AT_EGID, 1000);
//	NEW_AUXV_ENTRY(AT_SECURE, 0);
	NEW_AUXV_ENTRY(AT_RANDOM, random_ptr.Get());

	if(auxvs_.HWCAP2 != 0) {
		NEW_AUXV_ENTRY(AT_HWCAP2, auxvs_.HWCAP2);
	}

	NEW_AUXV_ENTRY(AT_EXECFN, argv_ptrs.at(0).Get());
	NEW_AUXV_ENTRY(AT_PLATFORM, platform_ptr.Get());

	NEW_AUXV_ENTRY(AT_NULL, 0);

	sp -= entry_size * elf_info_idx;
	int items = (argc + 1) + (envc + 1) + 1;
	sp = (sp - (items * entry_size)) & ~15UL;

	_initial_stack_pointer = sp;

#define WRITE_STACK(value) do { if(Is64BitBinary()) {GetMemoryModel().Write64(sp, value); sp += 8;} else { GetMemoryModel().Write32(sp, value); sp += 4;} } while(0)
	WRITE_STACK(argc);
	for(auto i : argv_ptrs) {
		WRITE_STACK(i.Get());
	}
	WRITE_STACK(0);
	for(auto i : envp_ptrs) {
		WRITE_STACK(i.Get());
	}
	WRITE_STACK(0);
	for(auto i : auxv_entries) {
		WRITE_STACK(i.first);
		WRITE_STACK(i.second);
	}

	return true;
}

struct LoadedBinaryInfo {
	bool Success;

	Address InitialBreak;
	Address EntryPoint;
	Address EntrySize;
	Address HeaderLocation;

	uint32_t HeaderEntryCount;
};

template<typename ElfClass> LoadedBinaryInfo Load_Binary(UserEmulationModel *model)
{
	LoadedBinaryInfo info;

	loader::UserElfBinaryLoader<ElfClass> elf_loader(*model, (true));

	// Load the binary.
	if (!elf_loader.LoadBinary(archsim::options::TargetBinary)) {
		LC_ERROR(LogEmulationModelUser) << "Unable to load binary: " << (std::string)archsim::options::TargetBinary;
		info.Success = false;
	} else {
		info.InitialBreak = elf_loader.GetInitialProgramBreak();
		info.EntryPoint = elf_loader.GetEntryPoint();
		info.EntrySize = elf_loader.GetProgramHeaderEntrySize();
		info.HeaderLocation = elf_loader.GetProgramHeaderLocation();
		info.HeaderEntryCount = elf_loader.GetProgramHeaderEntryCount();
		info.Success = true;
	}

	return info;
}

bool UserEmulationModel::PrepareBoot(System &system)
{
	LoadedBinaryInfo info;
	if(Is64BitBinary()) {
		info = Load_Binary<loader::ElfClass64>(this);
	} else {
		info = Load_Binary<loader::ElfClass32>(this);
	}

	if(!info.Success) {
		return false;
	}

	SetInitialBreak(info.InitialBreak);

	_initial_entry_point = Address(info.EntryPoint);
	LC_DEBUG1(LogEmulationModelUser) << "Located entry-point: " << std::hex << _initial_entry_point;

	// Initialise the program arguments.
	InitialiseProgramArguments();

	// Try and initialise the user-mode stack.
	if (!PrepareStack(system, info.HeaderLocation, info.HeaderEntryCount, info.EntrySize.Get()))
		return false;

	//TODO: Fix this
	/*
	if (_initial_entry_point & 1) {
		cpu->set_cpu_mode(1);
		cpu->write_register_T(1);
		_initial_entry_point &= ~1;
	} else {
		cpu->set_cpu_mode(0);
		cpu->write_register_T(0);
	}
	 * */

	LC_DEBUG1(LogEmulationModelUser) << "Initial stack pointer: " << std::hex << _initial_stack_pointer;

	GetMainThread()->SetPC(Address(_initial_entry_point));
	GetMainThread()->SetSP(Address(_initial_stack_pointer));
//	cpu->write_pc(_initial_entry_point);
//	cpu->write_sp(_initial_stack_pointer);

	return true;
}

bool UserEmulationModel::EmulateSyscall(SyscallRequest &request, SyscallResponse &response)
{
//	if (archsim::options::Verbose)
//		GetBootCore()->metrics.syscalls_invoked.inc();
	return syscall_handler_.HandleSyscall(request, response);
}

archsim::Address UserEmulationModel::MapAnonymousRegion(size_t size, archsim::abi::memory::RegionFlags flags)
{
	return Address(GetMemoryModel().GetMappingManager()->MapAnonymousRegion(size, flags));
}

bool UserEmulationModel::MapRegion(archsim::Address addr, size_t size, archsim::abi::memory::RegionFlags flags, const std::string &region_name)
{
	return GetMemoryModel().GetMappingManager()->MapRegion(addr, size, flags, region_name);
}

void UserEmulationModel::SetInitialBreak(Address brk)
{
	LC_DEBUG1(LogEmulationModelUser) << "Setting initial program break: " << std::hex << brk;
	_initial_program_break = brk;
	_program_break = brk;

	GetMemoryModel().GetMappingManager()->MapRegion(GetMemoryModel().AlignDown(_program_break), GetMemoryModel().AlignUp(Address(1)).Get(), (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[heap]");
}

void UserEmulationModel::SetBreak(Address brk)
{
	LC_DEBUG1(LogEmulationModelUser) << "Setting program break: " << std::hex << brk;

	if (brk < _initial_program_break) return;

	GetMemoryModel().GetMappingManager()->RemapRegion(_initial_program_break, GetMemoryModel().AlignUp(brk - _initial_program_break).Get());
	_program_break = brk;
}

Address UserEmulationModel::GetBreak()
{
	return _program_break;
}

Address UserEmulationModel::GetInitialBreak()
{
	return _initial_program_break;
}

ExceptionAction UserEmulationModel::HandleException(archsim::core::thread::ThreadInstance *thread, unsigned int category, unsigned int data)
{
	LC_WARNING(LogEmulationModelUser) << "Unhandled exception " << category << ", " << data;
	return AbortSimulation;
}
