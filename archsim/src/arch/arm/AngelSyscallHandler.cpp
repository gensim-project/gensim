/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * AngelSyscallHandler.cpp
 *
 *  Created on: 8 Sep 2014
 *      Author: harry
 */

#include "abi/memory/MemoryModel.h"

#include "arch/arm/AngelSyscallHandler.h"
#include "util/LogContext.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace archsim::arch::arm;

DeclareLogContext(LogAngel, "Angel");

AngelSyscallHandler::AngelSyscallHandler(archsim::abi::memory::MemoryModel &mem, uint32_t heap_base, uint32_t heap_limit, uint32_t stack_base, uint32_t stack_limit) : memory_model(mem), heap_base(heap_base), heap_limit(heap_limit), stack_base(stack_base), stack_limit(stack_limit)
{
	assert(heap_limit > heap_base);
	assert(stack_limit < stack_base);
}


bool AngelSyscallHandler::HandleSyscall(gensim::Processor &cpu)
{
	UNIMPLEMENTED;
//	const auto regs = cpu.GetRegisterBankDescriptor("RB");
//	uint32_t type = ((uint32_t*)(regs.GetBankDataStart()))[0];
//	uint32_t arg = ((uint32_t*)(regs.GetBankDataStart()))[1];
//	uint32_t &out = ((uint32_t*)(regs.GetBankDataStart()))[0];
//
//
//	switch(type) {
//		case 0x1: //SYS_OPEN
//			return HandleOpen(arg, out);
//		case 0x2: //SYS_CLOSE
//			return HandleClose(arg, out);
//		case 0x3: //SYS_WRITEC
//			return HandleWriteC(arg, out);
//		case 0x4: //SYS_WRITE0
//			return HandleWrite0(arg, out);
//		case 0x5:
//			return HandleWrite(arg, out);
//		case 0x6:
//			return HandleRead(arg, out);
//		case 0x9:
//			return HandleIsTty(arg, out);
//		case 0xa:
//			return HandleSeek(arg, out);
//		case 0xc:
//			return HandleFlen(arg, out);
//		case 0x10:
//			return HandleClock(arg, out);
//		case 0x12:
//			return HandleSystem(arg, out);
//		case 0x13:
//			return HandleErrno(arg, out);
//		case 0x15:
//			return HandleGetCmdline(arg, out);
//		case 0x16:
//			return HandleHeapInfo(arg, out);
//		case 0x18:
//			return HandleKill(arg, out);
//
//		default:
//			out = -1;
//			LC_ERROR(LogAngel) << "Could not handle unknown  request " << std::hex << type;
//			return false;
//	}
}



bool AngelSyscallHandler::HandleOpen(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t filename;
		uint32_t mode;
		uint32_t filename_length;
	} syscall_args;

	memory_model.ReadN(Address(arg), (uint8_t*)&syscall_args, sizeof(syscall_args));

	LC_DEBUG1(LogAngel) << "Handling open(" << syscall_args.filename << ", " << syscall_args.mode << ", " << syscall_args.filename_length << ")";

	char filename [syscall_args.filename_length+1];
	memory_model.ReadN(Address(syscall_args.filename), (uint8_t*)filename, syscall_args.filename_length);
	filename[syscall_args.filename_length] = 0;

	static constexpr const char* mode_strings[12] = {
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

	const char *modestr = mode_strings[syscall_args.mode];

	LC_DEBUG1(LogAngel) << "Translated to open (" << filename << ", " << modestr << ")";

	if(!strcmp(filename, ":tt")) {
		if(syscall_args.mode < 4) {
			LC_DEBUG1(LogAngel) << "Opening stdin";
			out = 0;
		} else {
			LC_DEBUG1(LogAngel) << "Opening stdout";
			out = 1;
		}
		return true;
	}

	FILE *file = fopen(filename, modestr);
	if(file) out = fileno(file);
	else out = -1;

	return true;
}

bool AngelSyscallHandler::HandleClose(uint32_t arg, uint32_t &out)
{
	uint32_t fd;

	memory_model.Read32(Address(arg), fd);

	LC_DEBUG1(LogAngel) << "Closing fd " << fd;

	//Don't let ANGEL close any standard outputs
	if(fd == 0 || fd == 1) return true;

	out = close(fd);
	return true;
}

bool AngelSyscallHandler::HandleWriteC(uint32_t arg, uint32_t &out)
{
	uint32_t ch;
	memory_model.Read32(Address(arg), ch);
	printf("%c", ch);

	return true;
}

bool AngelSyscallHandler::HandleWrite0(uint32_t arg, uint32_t &out)
{
	std::string str;
	unsigned char c;
	do {
		memory_model.Read8(Address(arg), c);
		str += c;
	} while(c != 0);

	printf("%s", str.c_str());
	return true;
}

bool AngelSyscallHandler::HandleWrite(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t handle;
		uint32_t data;
		uint32_t length;
	} syscall_args;

	memory_model.ReadN(Address(arg), (uint8_t*)&syscall_args, sizeof(syscall_args));

	LC_DEBUG1(LogAngel) << "Write (" << syscall_args.handle << ", " << syscall_args.data << ", " << syscall_args.length << ")";

	char *buffer = (char*)malloc(syscall_args.length);
	memory_model.ReadN(Address(syscall_args.data), (uint8_t*)buffer, syscall_args.length);

	out = write(syscall_args.handle, buffer, syscall_args.length);
	free(buffer);
	return true;
}

bool AngelSyscallHandler::HandleRead(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t handle;
		uint32_t buffer;
		uint32_t length;
		uint32_t mode; // ???
	} syscall_args;

	memory_model.ReadN(Address(arg), (uint8_t*)&syscall_args, sizeof(syscall_args));

	char *buffer = (char*)malloc(syscall_args.length);

	if(read(syscall_args.handle, buffer, syscall_args.length) != -1) {
		memory_model.WriteN(Address(syscall_args.buffer), (uint8_t*)buffer, syscall_args.length);
		out = 0;
	} else {
		out = syscall_args.length;
	}

	free(buffer);

	return true;
}

bool AngelSyscallHandler::HandleSeek(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t handle;
		uint32_t pos;
	} syscall_args;

	memory_model.ReadN(Address(arg), (uint8_t*)&syscall_args, sizeof(syscall_args));

	if(lseek(syscall_args.handle, syscall_args.pos, SEEK_SET) == -1)
		out = -1;
	else out = 0;

	return true;
}

bool AngelSyscallHandler::HandleFlen(uint32_t arg, uint32_t &out)
{
	uint32_t handle = arg;

	uint32_t prev_offset = lseek(handle, 0, SEEK_CUR);
	lseek(handle, 0, SEEK_END);
	uint32_t end = lseek(handle, 0, SEEK_CUR);
	lseek(handle, prev_offset, SEEK_SET);

	out = end;
	return true;
}

bool AngelSyscallHandler::HandleClock(uint32_t arg, uint32_t &out)
{
	out = 0;
	return true;
}

bool AngelSyscallHandler::HandleSystem(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t string;
		uint32_t length;
	} syscall_args;

	memory_model.ReadN(Address(arg), (uint8_t*)&syscall_args, sizeof(syscall_args));

	uint8_t *buffer = (uint8_t*)malloc(syscall_args.length);

	memory_model.ReadN(Address(syscall_args.string), buffer, syscall_args.length);
	LC_DEBUG1(LogAngel) << "Handling system(" << buffer << ");";

	free(buffer);
	return true;
}

bool AngelSyscallHandler::HandleErrno(uint32_t arg, uint32_t &out)
{
	out = errno;
	return true;
}

bool AngelSyscallHandler::HandleIsTty(uint32_t arg, uint32_t &out)
{
	uint32_t handle;
	memory_model.Read32(Address(arg), handle);

	if(isatty(handle)) out = -1;
	else out = 0;

	return true;
}

bool AngelSyscallHandler::HandleGetCmdline(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t buffer;
		uint32_t length;
	} info;

	const char cmdline[] = "some kind of command line";
	memory_model.ReadN(Address(arg), (uint8_t*)&info, sizeof(info));

	memory_model.WriteN(Address(info.buffer), (uint8_t*)cmdline, strlen(cmdline));
	info.length = strlen(cmdline);

	out = arg;
//		memory_model.WriteBlock(arg, &info, sizeof(info));

	//TODO write back the string length (somewhere)

	return true;
}

bool AngelSyscallHandler::HandleHeapInfo(uint32_t arg, uint32_t &out)
{
	struct {
		uint32_t heap_base;
		uint32_t heap_limit;
		uint32_t stack_base;
		uint32_t stack_limit;
	} heapinfo;

	uint32_t block_ptr;
	memory_model.Read32(Address(arg), block_ptr);
	memory_model.ReadN(Address(block_ptr), (uint8_t*)&heapinfo, sizeof(heapinfo));

	LC_DEBUG1(LogAngel) << "GetHeapInfo(" << arg << " -> " << block_ptr << ")";

	heapinfo.heap_base = heap_base;
	heapinfo.heap_limit = heap_limit;
	heapinfo.stack_base = stack_base;
	heapinfo.stack_limit = stack_limit;

	memory_model.WriteN(Address(block_ptr), (uint8_t*)&heapinfo, sizeof(heapinfo));

	out = arg;

	return true;
}

bool AngelSyscallHandler::HandleKill(uint32_t arg, uint32_t &out)
{
	memory_model.Read32(Address(arg), arg);
	LC_DEBUG1(LogAngel) << "Kill(" << arg << ")";
	// The simulation should only be able to kill itself. The easiest way to achieve that
	// is to just return false here.
	return false;
}
