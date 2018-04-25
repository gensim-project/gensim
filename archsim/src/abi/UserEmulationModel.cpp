#include "system.h"
#include "abi/UserEmulationModel.h"
#include "abi/loader/BinaryLoader.h"
#include "abi/user/SyscallHandler.h"
#include "util/SimOptions.h"
#include "util/CommandLine.h"
#include "gensim/gensim.h"
#include "gensim/gensim_processor.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "gensim/MemoryInterface.h"

extern char **environ;

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogEmulationModelUser, LogEmulationModel, "User");

using namespace archsim::abi;

UserEmulationModel::UserEmulationModel(const user::arch_descriptor_t &arch) : syscall_handler_(user::SyscallHandlerProvider::Singleton().Get(arch)) { }

UserEmulationModel::~UserEmulationModel() { }

void UserEmulationModel::PrintStatistics(std::ostream& stream)
{
//	cpu->PrintStatistics(stream);
	UNIMPLEMENTED;
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

	auto moduleentry = GetSystem().GetModuleManager().GetModule(archsim::options::ProcessorName)->GetEntry<archsim::module::ModuleExecutionEngineEntry>("EE");
	auto archentry = GetSystem().GetModuleManager().GetModule(archsim::options::ProcessorName)->GetEntry<archsim::module::ModuleArchDescriptorEntry>("ArchDescriptor");
	if(moduleentry == nullptr) {
		return false;
	}
	if(archentry == nullptr) {
		return false;
	}
	auto arch = archentry->Get();
	auto ctx = new archsim::ExecutionContext(*arch, moduleentry->Get());
	GetSystem().GetECM().AddContext(ctx);
	main_thread_ = new ThreadInstance(GetSystem().GetPubSub(), *arch, *this);
	
	for(auto i : main_thread_->GetMemoryInterfaces()) {
		i->Connect(*new archsim::LegacyMemoryInterface(GetMemoryModel()));
	}
	
	GetSystem().GetECM().AddContext(ctx);
	ctx->AddThread(main_thread_);
	
//	cpu = moduleentry->Get(archsim::options::ProcessorName, 0, &GetSystem().GetPubSub());

//	if (!cpu->Initialise(*this, GetMemoryModel())) {
//		return false;
//	}
//
//	cpu->reset_to_initial_state(true);

//	archsim::abi::devices::Device *coprocessor;
//	if(!GetComponentInstance("fpu", coprocessor)) return false;
//	cpu->peripherals.RegisterDevice("fpu", coprocessor);
//	cpu->peripherals.AttachDevice("fpu", 10);

#ifdef IO_DEVICES
	if (system.sim_opts.virtual_screen) {
		archsim::abi::devices::VirtualScreenDevice *vsd = new archsim::abi::devices::VirtualScreenDevice(*this);
		if (!vsd->Attach())
			return false;

		archsim::abi::devices::VirtualScreenDeviceKeyboardController *cont /* what did you call me */ = new archsim::abi::devices::VirtualScreenDeviceKeyboardController(*vsd);
		cpu->peripherals.RegisterDevice("kb", cont);
		cpu->peripherals.AttachDevice("kb", 13);
	}
#endif
	
	return true;
}

void UserEmulationModel::Destroy()
{
//	UNIMPLEMENTED;
}

gensim::Processor *UserEmulationModel::GetCore(int id)
{
	UNIMPLEMENTED;
}

archsim::ThreadInstance* UserEmulationModel::GetMainThread()
{
	return main_thread_;
}

void UserEmulationModel::ResetCores()
{
	UNIMPLEMENTED;
}

void UserEmulationModel::HaltCores()
{
	UNIMPLEMENTED;
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

bool UserEmulationModel::PrepareStack(System &system, loader::UserElfBinaryLoader &elf_loader)
{
	_initial_stack_pointer = 0xc0000000;
	_stack_size = archsim::options::GuestStackSize;

	if(_stack_size & ((1 << 12)-1)) {
		LC_ERROR(LogEmulationModelUser) << "[USER-EMULATION] Guest stack size must be a multiple of host page size";
		return false;
	}

	GetMemoryModel().GetMappingManager()->MapRegion(_initial_stack_pointer - _stack_size, _stack_size, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[stack]");

	unsigned long envp_ptrs[global_envc];
	unsigned long argv_ptrs[global_argc + 1];

	uint32_t sp = _initial_stack_pointer;

#define PUSH32(_val)   do { sp -= 4; GetMemoryModel().Write32(sp, _val); } while (0)
#define PUSHSTR(_str)  do { sp -= (strlen(_str) + 1); GetMemoryModel().WriteString(sp, _str); } while (0)
#define ALIGN_STACK(v) do { sp -= ((unsigned long)sp & (v - 1)); } while (0)
#define PUSH_AUX_ENT(_id, _value) do { PUSH32(_value); PUSH32(_id); } while (0)
#define STACK_ROUND(_sp, _items) (((unsigned long) (_sp - _items)) &~ 15UL)

	PUSH32(0);
	PUSHSTR(archsim::options::TargetBinary.GetValue().c_str()); // global_argv[0]);

	for (int i = 0; i < global_envc; i++) {
		if(environ[i][0] == '_')
			PUSHSTR("_=/home/a.out");
		else
			PUSHSTR(environ[i]);
		envp_ptrs[i] = sp;
	}

	PUSHSTR(archsim::options::TargetBinary.GetValue().c_str());  // The real arg0
	argv_ptrs[0] = sp;

	for (int i = 0; i < global_argc; i++) {
		PUSHSTR(global_argv[i]);
		argv_ptrs[i+1] = sp;
	}

	ALIGN_STACK(4);

	unsigned int elemSize = (global_argc + 2 + global_envc + 1 + 19) * 4;
	sp = (sp - ((sp - elemSize) - ((sp - elemSize) & (~15))));

	PUSH_AUX_ENT(0, 0);

	PUSH_AUX_ENT(3, elf_loader.GetProgramHeaderLocation());    // AT_PHDR
	PUSH_AUX_ENT(4, elf_loader.GetProgramHeaderEntrySize());   // AT_PHENT
	PUSH_AUX_ENT(5, elf_loader.GetProgramHeaderEntryCount());  // AT_PHNUM
	PUSH_AUX_ENT(6, 4096);					   // AT_PAGESZ

	PUSH_AUX_ENT(7, 0);			// AT_BASE
	PUSH_AUX_ENT(8, 0);			// AT_FLAGS
	PUSH_AUX_ENT(9, _initial_entry_point);  // Entry Point

	PUSH_AUX_ENT(11, 0);  // AT_UID
	PUSH_AUX_ENT(12, 0);  // AT_EUID
	PUSH_AUX_ENT(13, 0);  // AT_GID
	PUSH_AUX_ENT(14, 0);  // AT_EGID

	PUSH_AUX_ENT(16, 0x8197);	// AT_HWCAP
//	PUSH_AUX_ENT(17, sysconf(_SC_CLK_TCK));
	PUSH_AUX_ENT(23, 0);		 // AT_SECURE
	PUSH_AUX_ENT(25, 0xffff0000);    // AT_RANDOM
//	PUSH_AUX_ENT(26, 0x1f);    // AT_RANDOM
	PUSH_AUX_ENT(31, argv_ptrs[0]);  // AT_EXECFN

	// ENVP ARRAY
	PUSH32(0);
	for (int i = global_envc - 1; i >= 0; i--) PUSH32(envp_ptrs[i]);

	// ARGV ARRAY
	PUSH32(0);
	for (int i = global_argc; i >= 0; i--) PUSH32(argv_ptrs[i]);

	PUSH32(global_argc + 1);

	_initial_stack_pointer = sp;

	return true;
}

bool UserEmulationModel::PrepareBoot(System &system)
{
	loader::UserElfBinaryLoader elf_loader(*this, (true));

	// Load the binary.
	if (!elf_loader.LoadBinary(archsim::options::TargetBinary)) {
		LC_ERROR(LogEmulationModelUser) << "Unable to load binary: " << (std::string)archsim::options::TargetBinary;
		return false;
	}

	SetInitialBreak(elf_loader.GetInitialProgramBreak());

	_initial_entry_point = elf_loader.GetEntryPoint();
	LC_DEBUG1(LogEmulationModelUser) << "Located entry-point: " << std::hex << _initial_entry_point;

	// Initialise the program arguments.
	InitialiseProgramArguments();

	// Try and initialise the user-mode stack.
	if (!PrepareStack(system, elf_loader))
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
	return GetMemoryModel().GetMappingManager()->MapRegion(addr.Get(), size, flags, region_name);
}

void UserEmulationModel::SetInitialBreak(unsigned int brk)
{
	LC_DEBUG1(LogEmulationModelUser) << "Setting initial program break: " << std::hex << brk;
	_initial_program_break = brk;
	_program_break = brk;

	GetMemoryModel().GetMappingManager()->MapRegion(GetMemoryModel().AlignDown(_program_break), GetMemoryModel().AlignUp(1), (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[heap]");
}

void UserEmulationModel::SetBreak(unsigned int brk)
{
	LC_DEBUG1(LogEmulationModelUser) << "Setting program break: " << std::hex << brk;

	if (brk < _initial_program_break) return;

	GetMemoryModel().GetMappingManager()->RemapRegion(_initial_program_break, GetMemoryModel().AlignUp(brk - _initial_program_break));
	_program_break = brk;
}

unsigned int UserEmulationModel::GetBreak()
{
	return _program_break;
}

unsigned int UserEmulationModel::GetInitialBreak()
{
	return _initial_program_break;
}

ExceptionAction UserEmulationModel::HandleException(archsim::ThreadInstance *thread, unsigned int category, unsigned int data)
{
	LC_WARNING(LogEmulationModelUser) << "Unhandled exception " << category << ", " << data;
	return AbortSimulation;
}
