/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ArmAngelSemihostingEmulationModel.cpp
 *
 *  Created on: 13 Dec 2013
 *      Author: harry
 */

#include "gensim/gensim_processor.h"

#include "ioc/Context.h"

#include "system.h"
#include "gensim/gensim.h"
#include "abi/EmulationModel.h"
#include "util/ComponentManager.h"
#include "abi/loader/BinaryLoader.h"

#include "arch/Configuration.h"
#include "arch/SimOptions.h"
#include "arch/ModuleArch.h"
#include "arch/CoreArch.h"

class ArmAngelSemihostingEmulationModel;

RegisterComponent(archsim::abi::EmulationModel, ArmAngelSemihostingEmulationModel, "angel", "Arm Linux syscall emulation model", archsim::abi::memory::MemoryModel*);

using namespace archsim::abi;

class ArmAngelSemihostingEmulationModel : public archsim::abi::UserEmulationModel
{
public:

	gensim::Processor *cpu;
	uint32_t _initial_entry_point, _initial_stack_pointer, _stack_size;

	ArmAngelSemihostingEmulationModel(memory::MemoryModel *mem_model) :
		UserEmulationModel(mem_model),
		_stack_size(0x40000)
	{

	}

	bool Initialise(System& system, gensim::ProcLibCtx &proc_ctx)
	{
		archsim::ioc::Context *sys_ctx = archsim::ioc::Context::Global().create_context(0, archsim::ioc::Context::kNSystem);
		archsim::ioc::Context *mod_ctx = sys_ctx->create_context(0, archsim::ioc::Context::kNModule);
		archsim::ioc::Context *cpu_ctx = mod_ctx->create_context(0, archsim::ioc::Context::kNProcessor);

		ModuleArch *module_arch = system.sys_conf.sys_arch.module_type[0];
		CoreArch *core_arch = module_arch->core_type[0];

		cpu = gensim::Gensim::CreateProcessor(proc_ctx.procName, system, *core_arch, *cpu_ctx, *this, 0);
		cpu->reset_to_initial_state(true);

		return true;

	}

	void Destroy()
	{

	}

	void Reset()
	{
		cpu->reset_to_initial_state(true);
	}

	gensim::Processor* GetBootCore()
	{
		return cpu;
	}

	gensim::Processor* GetCore(int i)
	{
		return i ? NULL : cpu;
	}

	bool PrepareCores()
	{
		cpu->write_pc(_initial_entry_point);
		cpu->write_sp(_initial_stack_pointer);

		return true;
	}

	void ResetCores()
	{
		cpu->reset();
	}

	void HaltCores()
	{
		cpu->halt_cpu();
	}

	bool PrepareBoot(System& system, int argc, char** argv)
	{
		loader::UserElfBinaryLoader elf_loader(*this, true);

		// Load the binary.
		if (!elf_loader.LoadBinary(system.sim_opts.obj_name)) {
			LOG(LOG_ERROR) << "Unable to load binary: " << system.sim_opts.obj_name;
			return false;
		}

		_initial_stack_pointer = 0x40000000;
		memory_model.MapRegion(_initial_stack_pointer - _stack_size, _stack_size, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[stack]");

		_initial_entry_point = elf_loader.GetEntryPoint();
		LOG(LOG_DEBUG) << "Located entry-point: " << std::hex << _initial_entry_point;

		SetInitialBreak(elf_loader.GetInitialProgramBreak());

		return true;
	}

	bool InitialiseEmulationLayer()
	{
		return true;
	}

	bool BootSystem()
	{
		return true;
	}

	bool InstallDevices()
	{
		return true;
	}

	void DestroyDevices()
	{
		return;
	}

	ExceptionAction HandleOpen(uint32_t arg, uint32_t &out)
	{
		struct {
			uint32_t filename;
			uint32_t mode;
			uint32_t filename_length;
		} syscall_args;

		memory_model.ReadBlock(arg, &syscall_args, sizeof(syscall_args));

		LOG(LOG_DEBUG3) << "[SEMIHOSTING] Handling open(" << syscall_args.filename << ", " << syscall_args.mode << ", " << syscall_args.filename_length << ")";

		char filename [syscall_args.filename_length+1];
		memory_model.ReadBlock(syscall_args.filename, filename, syscall_args.filename_length);
		filename[syscall_args.filename_length] = 0;

		static constexpr char* mode_strings[12] = {
			"r",
			"rb",
			"r+",
			"r+b",
			"w",
			"wb",
			"w+",
			"w+b",
			"a",
			"ab",
			"a+",
			"a+b"
		};

		char *modestr = mode_strings[syscall_args.mode];

		LOG(LOG_DEBUG3) << "[SEMIHOSTING] Translated to open (" << filename << ", " << modestr << ")";

		if(!strcmp(filename, ":tt")) {
			if(syscall_args.mode < 4) {
				LOG(LOG_DEBUG3) << "Opening stdin";
				out = 0;
			} else {
				LOG(LOG_DEBUG3) << "Opening stdout";
				out = 1;
			}
			return ResumeNext;
		}

		FILE *file = fopen(filename, modestr);
		if(file) out = fileno(file);
		else out = -1;

		return ResumeNext;
	}

	ExceptionAction HandleClose(uint32_t arg, uint32_t &out)
	{
		uint32_t fd;

		memory_model.Read32(arg, fd);
		out = close(fd);
		return ResumeNext;
	}

	ExceptionAction HandleWriteC(uint32_t arg, uint32_t &out)
	{
		uint32_t ch;
		memory_model.Read32(arg, ch);
		printf("%c", ch);

		return ResumeNext;
	}

	ExceptionAction HandleWrite0(uint32_t arg, uint32_t &out)
	{
		std::string str;
		unsigned char c;
		do {
			memory_model.Read8(arg, c);
			str += c;
		} while(c != 0);

		printf("%s", str.c_str());
		return ResumeNext;
	}

	ExceptionAction HandleWrite(uint32_t arg, uint32_t &out)
	{
		struct {
			uint32_t handle;
			uint32_t data;
			uint32_t length;
		} syscall_args;

		memory_model.ReadBlock(arg, &syscall_args, sizeof(syscall_args));

		LOG(LOG_DEBUG3) << "[ANGEL] Write (" << syscall_args.handle << ", " << syscall_args.data << ", " << syscall_args.length << ")";

		char *buffer = (char*)malloc(syscall_args.length);
		memory_model.ReadBlock(syscall_args.data, buffer, syscall_args.length);

		out = write(syscall_args.handle, buffer, syscall_args.length);
		free(buffer);
		return ResumeNext;
	}

	ExceptionAction HandleRead(uint32_t arg, uint32_t &out)
	{
		struct {
			uint32_t handle;
			uint32_t buffer;
			uint32_t length;
			uint32_t mode; // ???
		} syscall_args;

		memory_model.ReadBlock(arg, &syscall_args, sizeof(syscall_args));

		char *buffer = (char*)malloc(syscall_args.length);

		if(read(syscall_args.handle, buffer, syscall_args.length) != -1) {
			memory_model.WriteBlock(syscall_args.buffer, buffer, syscall_args.length);
			out = 0;
		} else {
			out = syscall_args.length;
		}

		free(buffer);

		return ResumeNext;
	}

	ExceptionAction HandleSeek(uint32_t arg, uint32_t &out)
	{
		struct {
			uint32_t handle;
			uint32_t pos;
		} syscall_args;

		memory_model.ReadBlock(arg, &syscall_args, sizeof(syscall_args));

		if(lseek(syscall_args.handle, syscall_args.pos, SEEK_SET) == -1)
			out = -1;
		else out = 0;

		return ResumeNext;
	}

	ExceptionAction HandleFlen(uint32_t arg, uint32_t &out)
	{
		uint32_t handle = arg;

		uint32_t prev_offset = lseek(handle, 0, SEEK_CUR);
		lseek(handle, 0, SEEK_END);
		uint32_t end = lseek(handle, 0, SEEK_CUR);
		lseek(handle, prev_offset, SEEK_SET);

		out = end;
		return ResumeNext;
	}

	ExceptionAction HandleIsTty(uint32_t arg, uint32_t &out)
	{
		uint32_t handle;
		memory_model.Read32(arg, handle);

		if(isatty(handle)) out = -1;
		else out = 0;

		return ResumeNext;
	}

	ExceptionAction HandleGetCmdline(uint32_t arg, uint32_t &out)
	{
		struct {
			uint32_t buffer;
			uint32_t length;
		} info;

		const char cmdline[] = "some kind of command line";
		memory_model.ReadBlock(arg, &info, sizeof(info));

		memory_model.WriteString(info.buffer, cmdline);
		info.length = strlen(cmdline);

		out = arg;
//		memory_model.WriteBlock(arg, &info, sizeof(info));

		//TODO write back the string length (somewhere)

		return ResumeNext;
	}

	ExceptionAction HandleHeapInfo(uint32_t arg, uint32_t &out)
	{
		struct {
			uint32_t heap_base;
			uint32_t heap_limit;
			uint32_t stack_base;
			uint32_t stack_limit;
		} heapinfo;

		uint32_t block_ptr;
		memory_model.Read32(arg, block_ptr);
		memory_model.ReadBlock(block_ptr, &heapinfo, sizeof(heapinfo));

		LOG(LOG_DEBUG3) << "[SEMIHOSTING] GetHeapInfo(" << arg << " -> " << block_ptr << ")";

		heapinfo.heap_base = GetBreak();
		heapinfo.heap_limit = GetBreak() + 0x8000;
		heapinfo.stack_base = _initial_stack_pointer;
		heapinfo.stack_limit = _initial_stack_pointer - 0x10000;

		memory_model.WriteBlock(block_ptr, &heapinfo, sizeof(heapinfo));

		out = arg;

		return ResumeNext;
	}

	ExceptionAction HandleException(gensim::Processor &cpu, uint32_t category, uint32_t data)
	{
		LOG(LOG_DEBUG1) << "[SEMIHOSTING] Handling an exception " << category << " " << data;
		if(data == 0x123456 || data == 0xab) {
			const auto regs = cpu.GetRegisterBankDescriptor("RB");
			uint32_t type = ((uint32_t*)(regs.BankDataStart))[0];
			uint32_t arg = ((uint32_t*)(regs.BankDataStart))[1];
			uint32_t &out = ((uint32_t*)(regs.BankDataStart))[0];

			LOG(LOG_DEBUG1) << "[SEMIHOSTING] Angel call " << type << "(" << arg << ")";

			switch(type) {
				case 0x1: //SYS_OPEN
					return HandleOpen(arg, out);
				case 0x2: //SYS_CLOSE
					return HandleClose(arg, out);
				case 0x3: //SYS_WRITEC
					return HandleWriteC(arg, out);
				case 0x4: //SYS_WRITE0
					return HandleWrite0(arg, out);
				case 0x5:
					return HandleWrite(arg, out);
				case 0x6:
					return HandleRead(arg, out);
				case 0x9:
					return HandleIsTty(arg, out);
				case 0xa:
					return HandleSeek(arg, out);
				case 0xc:
					return HandleFlen(arg, out);
				case 0x15:
					return HandleGetCmdline(arg, out);
				case 0x16:
					return HandleHeapInfo(arg, out);

				default:
					LOG(LOG_ERROR) << "[SEMIHOSTING] Unimplemented ANGEL syscall " << type;
					out = -1;
					return ResumeNext;
			}
		}

		return ResumeNext;
	}
};
