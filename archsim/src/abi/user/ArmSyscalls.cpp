/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * abi/user/ArmSyscalls.cpp
 */
#include "system.h"

#include "abi/user/SyscallHandler.h"
#include "abi/user/ArmSyscalls.h"
#include "abi/EmulationModel.h"
#include "abi/UserEmulationModel.h"
#include "abi/memory/MemoryModel.h"
#include "core/MemoryInterface.h"
#include <sys/utsname.h>

#include "util/SimOptions.h"
#include "util/LogContext.h"

#include <asm/prctl.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/times.h>

#include "termios.h"

UseLogContext(LogSyscalls);

using archsim::Address;

static unsigned long syscall(unsigned int syscall_no, unsigned long a0 = 0, unsigned long a1 = 0, unsigned long a2 = 0, unsigned long a3 = 0, unsigned long a4 = 0, unsigned long a5 = 0)
{
	unsigned long result;

	asm volatile (
	    "mov %4, %%r10"
	    "mov %5, %%r9"
	    "mov %6, %%r8"
	    "syscall" : "=a"(result) : "D"(a0), "S"(a1), "d"(a2), "m"(a3), "m"(a4), "m"(a5) : "r8", "r9", "r10");

	return result;
}

static unsigned int translate_fd(archsim::core::thread::ThreadInstance* cpu, int fd)
{
	fd = cpu->GetEmulationModel().GetSystem().GetFD(fd);

	return fd;
}

static unsigned int sys_exit(archsim::core::thread::ThreadInstance* cpu, unsigned int exit_code)
{
	cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
	cpu->GetEmulationModel().GetSystem().exit_code = exit_code;

	return 0;
}

static unsigned long sys_uname(archsim::core::thread::ThreadInstance* cpu, unsigned long addr)
{
	if (addr == 0) {
		return -EFAULT;
	}
	auto interface = cpu->GetMemoryInterface(0);

	interface.WriteString(Address(addr), "Linux");

	addr += 64;
	interface.WriteString(Address(addr), "archsim");

	addr += 64;
	interface.WriteString(Address(addr), "4.8.0");

	addr += 64;
	interface.WriteString(Address(addr), "#1");

	addr += 64;
	// HACK:
	interface.WriteString(Address(addr), "arm64");

	return 0;
}
static unsigned long sys_uname_x86(archsim::core::thread::ThreadInstance* cpu, unsigned long addr)
{
	LC_DEBUG1(LogSyscalls) << "Uname: addr=" << Address(addr);
	if (addr == 0) {
		return -EFAULT;
	}
	auto interface = cpu->GetMemoryInterface(0);

	interface.WriteString(Address(addr), "Linux");

	addr += 65;
	interface.WriteString(Address(addr), "archsim");

	addr += 65;
	interface.WriteString(Address(addr), "4.16.11-100.fc26.x86_64");

	addr += 65;
	interface.WriteString(Address(addr), "#1 SMP Tue May 22 20:02:12 UTC 2018");

	addr += 65;
	// HACK:
	interface.WriteString(Address(addr), "x86_64");

	return 0;
}

static unsigned long sys_open(archsim::core::thread::ThreadInstance* cpu, unsigned long filename_addr, unsigned int flags, unsigned int mode)
{
	LC_DEBUG1(LogSyscalls) << "open " << Address(filename_addr) << " flags=" << flags << ", mode=" << mode;
	char filename[256];
	auto interface = cpu->GetMemoryInterface(0);
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);
	LC_DEBUG1(LogSyscalls) << "opening " << filename;

	int host_fd = open(filename, flags, mode);
	if (host_fd < 0) return -errno;

	int guest_fd = cpu->GetEmulationModel().GetSystem().OpenFD(host_fd);

	LC_DEBUG1(LogSyscalls) << "Open: Opened " << filename << " with host FD " << host_fd << ", guest FD " << guest_fd;

	return guest_fd;
}

static unsigned long sys_openat(archsim::core::thread::ThreadInstance* cpu, int dirfd, unsigned long filename_addr, unsigned int flags, unsigned int mode)
{
	if(dirfd == AT_FDCWD) {
		return sys_open(cpu, filename_addr, flags, mode);
	}

	auto interface = cpu->GetMemoryInterface(0);
	char filename[256];
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);

	int host_fd = openat(translate_fd(cpu, dirfd),filename, flags, mode);
	if (host_fd < 0) return -errno;

	int guest_fd = cpu->GetEmulationModel().GetSystem().OpenFD(host_fd);

	return guest_fd;
}

static unsigned long sys_close(archsim::core::thread::ThreadInstance* cpu, unsigned int fd)
{
	if (cpu->GetEmulationModel().GetSystem().CloseFD(fd))
		return -errno;
	return 0;
}

template<typename guest_iovec> static unsigned long sys_writev(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned long iov_addr, int cnt)
{
	LC_DEBUG1(LogSyscalls) << "writev " << fd << " " << Address(iov_addr) << " " << cnt;
	if (cnt < 0) {
		return -1;
	}

	struct iovec host_vectors[cnt];
	guest_iovec arm_vectors[cnt];

	auto interface = cpu->GetMemoryInterface(0);
	interface.Read(Address(iov_addr), (uint8_t *)&arm_vectors, cnt * sizeof(guest_iovec));

	for (int i = 0; i < cnt; i++) {
		host_vectors[i].iov_base = new char[arm_vectors[i].iov_len];
		host_vectors[i].iov_len = arm_vectors[i].iov_len;
		interface.Read(Address(arm_vectors[i].iov_base), (uint8_t *)host_vectors[i].iov_base, arm_vectors[i].iov_len);
	}

	fd = cpu->GetEmulationModel().GetSystem().GetFD(fd);
	fsync(fd);  // HACK?
	unsigned int res = (uint32_t)writev(fd, host_vectors, cnt);
	fsync(fd);  // HACK?

	for (int i = 0; i < cnt; i++) {
		delete[](char*)host_vectors[i].iov_base;
	}

	return res;
}

static unsigned long sys_lseek(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned int offset, unsigned int whence)
{
	fd = translate_fd(cpu, fd);
	int lseek_result = lseek(fd, offset, whence);
	if (lseek_result < 0)
		return -errno;
	else
		return lseek_result;
}

static unsigned long sys_llseek(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned int offset_high, unsigned int offset_low, unsigned int result_addr, unsigned int whence)
{
	loff_t result;

	auto interface = cpu->GetMemoryInterface(0);

	fd = translate_fd(cpu, fd);
	uint64_t offset;
	if(archsim::options::ArmOabi)
		offset = ((((uint64_t)offset_low) << 32) | (uint64_t)offset_high);
	else
		offset = ((((uint64_t)offset_high) << 32) | (uint64_t)offset_low);

	result = lseek64(fd, offset, whence);

	if ((result != -1) && result_addr) {
		interface.Write64(Address(result_addr), (uint64_t)result);
	}

	return result == -1 ? -errno : 0;
}

static unsigned long sys_unlink(archsim::core::thread::ThreadInstance* cpu, unsigned long filename_addr)
{
	char filename[256];
	auto interface = cpu->GetMemoryInterface(0);
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);

	if (unlink(filename)) return -errno;

	return 0;
}

static unsigned long sys_mmap(archsim::core::thread::ThreadInstance* cpu, unsigned long addr, unsigned int len, unsigned int prot, unsigned int flags, unsigned int fd, unsigned int off)
{
	LC_DEBUG1(LogSyscalls) << "MMAP: " << Address(addr) << ", len=" << len << ", prot=" << prot << ", flags=" << flags << ", fd=" << fd << ", off=" << off;
	archsim::abi::memory::RegionFlags reg_flags = (archsim::abi::memory::RegionFlags)(archsim::abi::memory::RegFlagRead | archsim::abi::memory::RegFlagWrite);
	if(prot & PROT_EXEC) reg_flags = (archsim::abi::memory::RegionFlags)(reg_flags | archsim::abi::memory::RegFlagExecute);

	auto &emu_model = (archsim::abi::UserEmulationModel&)cpu->GetEmulationModel();
	if (addr == 0 && !(flags & MAP_FIXED)) {
		addr = emu_model.MapAnonymousRegion(len, reg_flags).Get();
	} else {
		if (!emu_model.MapRegion(Address(addr), len, reg_flags, ""))
			return -1;
	}

	// 'fake' mmap of file
	if(!(flags & MAP_ANONYMOUS)) {
		int host_fd = cpu->GetEmulationModel().GetSystem().GetFD(fd);
		size_t offset = lseek(host_fd, 0, SEEK_CUR);

		lseek(host_fd, off, SEEK_SET);
		std::vector<char> buf(len, 0);
		read(host_fd, buf.data(), len);
		cpu->GetMemoryInterface(0).Write(Address(addr), (uint8_t*)buf.data(), len);
		lseek(host_fd, offset, SEEK_SET);
	}

	return addr;
}

static unsigned long sys_mmap2(archsim::core::thread::ThreadInstance* cpu, unsigned long addr, unsigned int len, unsigned int prot, unsigned int flags, unsigned int fd, unsigned int off)
{
	return sys_mmap(cpu, addr, len, prot, flags, fd, off);
}

static unsigned long sys_mremap(archsim::core::thread::ThreadInstance* cpu, unsigned long old_addr, unsigned int old_size, unsigned int new_size, unsigned int flags)
{
	LC_ERROR(LogSyscalls) << "mremap not supported";
	return -1;
}

static unsigned long sys_munmap(archsim::core::thread::ThreadInstance* cpu, unsigned long addr, unsigned int len)
{
//	UNIMPLEMENTED;
//	if (!cpu.GetMemoryModel().IsAligned(len)) {
//		return -EINVAL;
//	}
//
//	cpu.GetMemoryModel().GetMappingManager()->UnmapRegion(addr, len);
	return 0;
}

static unsigned long sys_mprotect(archsim::core::thread::ThreadInstance *cpu, unsigned long addr, unsigned int len, unsigned int flags)
{
	LC_ERROR(LogSyscalls) << "[SYSCALL] mprotect not currently supported";
	LC_ERROR(LogSyscalls) << "[SYSCALL] " << std::hex << addr << " " << len << " " << flags;
	return 0;
}

static unsigned long sys_read(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned long addr, unsigned int len)
{
	char* rd_buf = new char[len];
	ssize_t res;

	fd = translate_fd(cpu, fd);
	res = read(fd, (void*)rd_buf, len);

	auto interface = cpu->GetMemoryInterface(0);

	if (res > 0) {
		interface.Write(Address(addr), (uint8_t *)rd_buf, res);
	}

	delete[] rd_buf;

	if (res < 0)
		return -errno;
	else
		return res;
}

static unsigned long sys_write(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned long addr, unsigned int len)
{
	ssize_t res;

	LC_DEBUG1(LogSyscalls) << "write: Writing to FD " << fd << ", from address " << Address(addr) << ", " << len << " bytes";

	if(len > 0x1000000) {
		cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
		return -EINVAL;
	}

	char *buffer = (char *)malloc(len);
	bzero(buffer, len);

	auto interface = cpu->GetMemoryInterface(0);
	if (interface.Read(Address(addr), (uint8_t *)buffer, len) != archsim::MemoryResult::OK) {
		free(buffer);
		return -EFAULT;
	}

	fd = translate_fd(cpu, fd);
	res = write(fd, buffer, len);
	free(buffer);



	if (res < 0) {
		res = -errno;
	}

	return res;
}

static unsigned long sys_fstat64(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned long addr)
{
	struct stat64 st;
	struct arm_stat64 result_st;

//	printf("Doing an fstat! %u %lx\n", fd, (uint64_t)addr);

	fd = translate_fd(cpu, fd);
	if (fstat64(fd, &st)) {
		return -errno;
	}
//	fprintf(stderr, "Host fd %u\n", fd);

	memset(&result_st, 0, sizeof(result_st));

	result_st.st_dev = st.st_dev;
//	printf("st_dev %lx\n", result_st.st_dev);
	result_st.st_ino = result_st.__st_ino = st.st_ino;
//	printf("st_ino %lx\n", result_st.st_ino);
	result_st.st_mode = st.st_mode;
//	printf("st_mode %lx\n", result_st.st_mode);
	result_st.st_nlink = st.st_nlink;
//	printf("st_nlink %lx\n", result_st.st_nlink);
	result_st.st_uid = st.st_uid;
//	printf("st_uid %lx\n", result_st.st_uid);
	result_st.st_gid = st.st_gid;
//	printf("st_gid %lx\n", result_st.st_gid);
	result_st.st_rdev = st.st_rdev;
//	printf("st_rdev %lx\n", result_st.st_rdev);
	result_st.st_size = st.st_size;
//	printf("st_size %lx\n", result_st.st_size);
	result_st.st_blocks = st.st_blocks;
//	printf("st_blocks %lx\n", result_st.st_blocks);
	result_st.target_st_atime = (uint32_t)st.st_atime;
//	printf("st_atime %lx\n", result_st.target_st_atime);
	result_st.target_st_mtime = (uint32_t)st.st_mtime;
//	printf("st_mtime %lx\n", result_st.target_st_mtime);
	result_st.target_st_ctime = (uint32_t)st.st_ctime;
//	printf("st_ctime %lx\n", result_st.target_st_ctime);
	result_st.st_blksize = st.st_blksize;
//	printf("st_blksize %lx\n", result_st.st_blksize);

	auto interface = cpu->GetMemoryInterface(0);
	interface.Write(Address(addr), (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned long sys_fstat(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned long addr)
{
	struct stat st;
	struct arm_stat result_st;

	fd = translate_fd(cpu, fd);
	if (fstat(fd, &st)) {
		return -errno;
	}

	memset(&result_st, 0, sizeof(result_st));

	printf("Doing an fstat! %u %lx\n", fd, (uint64_t)addr);

	result_st.st_dev = st.st_dev;
	printf("st_dev %lx\n", result_st.st_dev);
	result_st.__st_ino = st.st_ino;
	printf("st_ino %lx\n", result_st.__st_ino);
	result_st.st_mode = st.st_mode;
	printf("st_mode %lx\n", result_st.st_mode);
	result_st.st_nlink = st.st_nlink;
	printf("st_nlink %lx\n", result_st.st_nlink);
	result_st.st_uid = st.st_uid;
	printf("st_uid %lx\n", result_st.st_uid);
	result_st.st_gid = st.st_gid;
	printf("st_gid %lx\n", result_st.st_gid);
	result_st.st_rdev = st.st_rdev;
	printf("st_rdev %lx\n", result_st.st_rdev);
	result_st.st_size = st.st_size;
	printf("st_size %lx\n", result_st.st_size);
	result_st.st_blocks = st.st_blocks;
	printf("st_blocks %lx\n", result_st.st_blocks);
	result_st.target_st_atime = (uint32_t)st.st_atime;
	printf("st_atime %lx\n", result_st.target_st_atime);
	result_st.target_st_mtime = (uint32_t)st.st_mtime;
	printf("st_mtime %lx\n", result_st.target_st_mtime);
	result_st.target_st_ctime = (uint32_t)st.st_ctime;
	printf("st_ctime %lx\n", result_st.target_st_ctime);
	result_st.st_blksize = 4096;
	printf("st_blksize %lx\n", result_st.st_blksize);

	auto interface = cpu->GetMemoryInterface(0);
	interface.Write(Address(addr), (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned long sys64_fstat(archsim::core::thread::ThreadInstance *cpu, unsigned int fd, unsigned long addr)
{
	struct stat st;
	struct aarch64_stat result_st;

	fd = translate_fd(cpu, fd);
	if (fstat(fd, &st)) {
		return -errno;
	}

	memset(&result_st, 0, sizeof(result_st));

//	result_st.st_dev = st.st_dev;
//	result_st.__st_ino = st.st_ino;
	result_st.st_mode = st.st_mode;
//	result_st.st_nlink = st.st_nlink;
//	result_st.st_uid = st.st_uid;
//	result_st.st_gid = st.st_gid;
//	result_st.st_rdev = st.st_rdev;
//	result_st.st_size = st.st_size;
//	result_st.st_blksize = st.st_blksize;
//	result_st.st_blocks = st.st_blocks;
//	result_st.target_st_atime = st.st_atim.tv_sec;
//	result_st.st_atime_nsec = st.st_atim.tv_nsec;
//	result_st.target_st_mtime = st.st_mtim.tv_sec;
//	result_st.st_mtime_nsec = st.st_mtim.tv_nsec;
//	result_st.target_st_ctime = st.st_ctim.tv_sec;
//	result_st.st_ctime_nsec = st.st_ctim.tv_nsec;
//	result_st.st_blksize = 4096;

	auto interface = cpu->GetMemoryInterface(0);
	interface.Write(Address(addr), (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned long sys_stat64(archsim::core::thread::ThreadInstance* cpu, unsigned long filename_addr, unsigned long addr)
{
	struct stat64 st;
	struct arm_stat64 result_st;

	char filename[256];
	auto interface = cpu->GetMemoryInterface(0);
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);

	if (stat64(filename, &st)) {
		return -errno;
	}

	memset(&result_st, 0, sizeof(result_st));

	result_st.st_dev = st.st_dev;
	result_st.st_ino = result_st.__st_ino = st.st_ino;
	result_st.st_mode = st.st_mode;
	result_st.st_nlink = st.st_nlink;
	result_st.st_uid = st.st_uid;
	result_st.st_gid = st.st_gid;
	result_st.st_rdev = st.st_rdev;
	result_st.st_size = st.st_size;
	result_st.st_blocks = st.st_blocks;
	result_st.target_st_atime = (uint32_t)st.st_atime;
	result_st.target_st_mtime = (uint32_t)st.st_mtime;
	result_st.target_st_ctime = (uint32_t)st.st_ctime;
	result_st.st_blksize = 4096;

	interface.Write(Address(addr), (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned long sys_lstat64(archsim::core::thread::ThreadInstance* cpu, unsigned long filename_addr, unsigned long addr)
{
	struct stat64 st;
	struct arm_stat64 result_st;

	char filename[256];
	auto interface = cpu->GetMemoryInterface(0);
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);

	if (lstat64(filename, &st)) {
		return -errno;
	}

	memset(&result_st, 0, sizeof(result_st));

	result_st.st_dev = st.st_dev;
	result_st.st_ino = result_st.__st_ino = st.st_ino;
	result_st.st_mode = st.st_mode;
	result_st.st_nlink = st.st_nlink;
	result_st.st_uid = st.st_uid;
	result_st.st_gid = st.st_gid;
	result_st.st_rdev = st.st_rdev;
	result_st.st_size = st.st_size;
	result_st.st_blocks = st.st_blocks;
	result_st.target_st_atime = (uint32_t)st.st_atime;
	result_st.target_st_mtime = (uint32_t)st.st_mtime;
	result_st.target_st_ctime = (uint32_t)st.st_ctime;
	result_st.st_blksize = 4096;

	interface.Write(Address(addr), (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned long sys_lstat_x86(archsim::core::thread::ThreadInstance* cpu, unsigned long filename_addr, unsigned long addr)
{
	struct stat64 st;
	struct x86_64_stat64 result_st;

	char filename[256];
	auto interface = cpu->GetMemoryInterface(0);
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);

	if (lstat64(filename, &st)) {
		return -errno;
	}

	result_st.st_atim = st.st_atim;
	result_st.st_mtim = st.st_mtim;
	result_st.st_ctim = st.st_ctim;

//	result_st.st_attr = st.st_attr;
	result_st.st_blksize = st.st_blksize;
	result_st.st_blocks = st.st_blocks;
	result_st.st_dev = st.st_dev;
	result_st.st_gid = st.st_gid;
	result_st.st_ino = st.st_ino;
	result_st.st_mode = st.st_mode;
	result_st.st_nlink = st.st_nlink;
	result_st.st_rdev = st.st_rdev;
	result_st.st_size = st.st_size;
	result_st.st_uid = st.st_uid;

	interface.Write(Address(addr), (uint8_t*)&result_st, sizeof(result_st));
	return 0;
}

static unsigned long sys_ioctl(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned int request, unsigned long a0)
{
	fd = translate_fd(cpu, fd);
	auto interface = cpu->GetMemoryInterface(0);

	switch (request) {
		case 0x5401: {
			struct termios host;
			unsigned int rc = ioctl(fd, request, &host);

			if (rc) {
				return -errno;
			}

// HACK:
//			interface.Write(Address(a0), (uint8_t *)&host, sizeof(host));
			return 0;
		}

		default:
			fprintf(stderr, "ioctl: %d NOT IMPLEMENTED\n", request);
			return -ENOSYS;
	}
}

static unsigned long sys_fcntl64(archsim::core::thread::ThreadInstance* cpu, unsigned int fd, unsigned int cmd, unsigned long a0)
{
	int rc = fcntl(translate_fd(cpu, fd), cmd, a0);
	if (rc) return -errno;
	return 0;
}

static unsigned long sys_mkdir(archsim::core::thread::ThreadInstance* cpu, unsigned long filename_addr, unsigned int mode)
{
	char filename[256];
	auto interface = cpu->GetMemoryInterface(0);
	interface.ReadString(Address(filename_addr), filename, sizeof(filename) - 1);

	LC_DEBUG1(LogSyscalls) << "mkdir addr=" << Address(filename_addr) << " (" << filename << "), mode=" << mode;

	int rc = mkdir(filename, mode);
	if (rc) return -errno;

	return 0;
}

static unsigned long sys_getcwd(archsim::core::thread::ThreadInstance* cpu, unsigned long buffer_addr, unsigned int size)
{
	const unsigned int max_size = 4096;
	if (size == 0 || size > max_size)
		return -EINVAL;

	char *cwd = (char *)malloc(size + 1);

	if (getcwd(cwd, size) == NULL) {
		free(cwd);
		return -errno;
	}

	cwd[size] = '\0';
	auto interface = cpu->GetMemoryInterface(0);
	interface.WriteString(Address(buffer_addr), cwd);
	free(cwd);

	return 0;
}

static unsigned long sys_arm_settls(archsim::core::thread::ThreadInstance* cpu, unsigned long addr)
{
	LC_DEBUG1(LogSyscalls) << "TLS Set to 0x" << std::hex << addr;

	auto interface = cpu->GetMemoryInterface(0);
	interface.Write(Address(0xffff0ff0), (uint8_t *)&addr, sizeof(addr));

	// also need to write the TPIDRURO control register which is an alternative way of accessing the TLS value
	*cpu->GetRegisterFileInterface().GetEntry<uint32_t>("TPIDRURO") = addr;

	return 0;
}

static unsigned long sys_brk(archsim::core::thread::ThreadInstance* cpu, unsigned long new_brk)
{
	archsim::abi::UserEmulationModel& uem = static_cast<archsim::abi::UserEmulationModel&>(cpu->GetEmulationModel());
	auto oldbrk = uem.GetBreak();
	if (new_brk >= uem.GetInitialBreak()) uem.SetBreak(Address(new_brk));

	LC_DEBUG1(LogSyscalls) << "BRK: old=" << std::hex << oldbrk << ", requested=" << std::hex << new_brk << ", new=" << std::hex << uem.GetBreak();
	return uem.GetBreak().Get();
}

static unsigned long sys_gettimeofday(archsim::core::thread::ThreadInstance* cpu, unsigned long tv_addr, unsigned long tz_addr)
{
	struct timeval host_tv;
	struct timezone host_tz;

	struct arm_timeval guest_tv;
	auto interface = cpu->GetMemoryInterface(0);

	if (tv_addr == 0) {
		return -EFAULT;
	}

	if(archsim::options::Verify) {
		guest_tv.tv_sec = (uint32_t)0;
		guest_tv.tv_usec = (uint32_t)0;

		interface.Write(Address(tv_addr), (uint8_t *)&guest_tv, sizeof(guest_tv));
		return 0;
	}

	if (gettimeofday(&host_tv, &host_tz)) {
		return -errno;
	}

	guest_tv.tv_sec = (uint32_t)host_tv.tv_sec;
	guest_tv.tv_usec = (uint32_t)host_tv.tv_usec;

	interface.Write(Address(tv_addr), (uint8_t *)&guest_tv, sizeof(guest_tv));

	return 0;
}

static unsigned long sys_rt_sigprocmask(archsim::core::thread::ThreadInstance* cpu, unsigned int how, unsigned int set_ptr, unsigned int oldset_ptr)
{
	return 0;
}

static unsigned long sys_rt_sigaction(archsim::core::thread::ThreadInstance* cpu, unsigned int signum, unsigned int act_ptr, unsigned int oldact_ptr)
{
	if(!archsim::options::UserPermitSignalHandling) return -EINVAL;
	auto interface = cpu->GetMemoryInterface(0);
	if (act_ptr == 0) {
		if (oldact_ptr == 0) {
			return -EINVAL;
		} else {
			archsim::abi::SignalData *data;
			if (!cpu->GetEmulationModel().GetSignalData(signum, data))
				return 0;

			interface.Write(Address(oldact_ptr), (uint8_t *)data->priv, sizeof(struct arm_sigaction));
			return 0;
		}
	} else {
		archsim::abi::SignalData *data = NULL;
		void *act = NULL;
		if (!cpu->GetEmulationModel().GetSignalData(signum, data)) {
			act = malloc(sizeof(struct arm_sigaction));
			if (act == NULL)
				return -EFAULT;
		} else {
			assert(data);
			act = data->priv;
		}

		interface.Read(Address(act_ptr), (uint8_t *)act, sizeof(struct arm_sigaction));
		cpu->GetEmulationModel().CaptureSignal(signum, ((struct arm_sigaction *)act)->sa_handler, act);
	}

	return 0;
}

static unsigned long sys_times(archsim::core::thread::ThreadInstance* cpu, unsigned long buf_addr)
{
	if (buf_addr == 0)
		return -EFAULT;

	struct tms host_buf;
	struct arm_tms guest_buf;
	auto interface = cpu->GetMemoryInterface(0);

	if(archsim::options::Verify) {
		guest_buf.tms_utime = (uint32_t)0;
		guest_buf.tms_stime = (uint32_t)0;
		guest_buf.tms_cutime = (uint32_t)0;
		guest_buf.tms_cstime = (uint32_t)0;

		interface.Write(Address(buf_addr), (uint8_t *)&guest_buf, sizeof(guest_buf));

		return 0;
	}

	if (times(&host_buf))
		return -errno;

	guest_buf.tms_utime = (uint32_t)host_buf.tms_utime;
	guest_buf.tms_stime = (uint32_t)host_buf.tms_stime;
	guest_buf.tms_cutime = (uint32_t)host_buf.tms_cutime;
	guest_buf.tms_cstime = (uint32_t)host_buf.tms_cstime;

	interface.Write(Address(buf_addr), (uint8_t *)&guest_buf, sizeof(guest_buf));

	return 0;
}

static void arm_timespec_to_host(struct arm_timespec *arm, struct timespec *ts)
{
	ts->tv_nsec = arm->tv_nsec;
	ts->tv_sec = arm->tv_sec;
}

static void host_timespec_to_arm(struct timespec *ts, struct arm_timespec *arm)
{
	arm->tv_nsec = ts->tv_nsec;
	arm->tv_sec = ts->tv_sec;
}

static unsigned long sys_clock_gettime(archsim::core::thread::ThreadInstance *cpu, unsigned int clk_id, unsigned long timespec_ptr)
{
	struct arm_timespec arm_ts;
	struct timespec ts;
	auto interface = cpu->GetMemoryInterface(0);

	if(archsim::options::Verify) {
		arm_ts.tv_nsec = 0;
		arm_ts.tv_sec = 0;
		interface.Write(Address(timespec_ptr), (uint8_t*)&arm_ts, sizeof(arm_ts));
		return 0;
	}

	int result = clock_gettime(clk_id, &ts);
	host_timespec_to_arm(&ts, &arm_ts);

	interface.Write(Address(timespec_ptr), (uint8_t*)&arm_ts, sizeof(arm_ts));

	return -result;
}

static unsigned long sys_nanosleep(archsim::core::thread::ThreadInstance* cpu, unsigned long req_ptr, unsigned long rem_ptr)
{
	struct arm_timespec arm_req, arm_rem;
	struct timespec req, rem;
	int rc;

	auto interface = cpu->GetMemoryInterface(0);

	bzero(&req, sizeof(req));
	bzero(&rem, sizeof(rem));
	bzero(&arm_req, sizeof(arm_req));
	bzero(&arm_rem, sizeof(arm_rem));

	interface.Read(Address(req_ptr), (uint8_t *)&arm_req, sizeof(arm_req));
	arm_timespec_to_host(&arm_req, &req);

	rc = nanosleep(&req, &rem);

	if (rem_ptr != 0) {
		host_timespec_to_arm(&rem, &arm_rem);
		interface.Write(Address(rem_ptr), (uint8_t *)&arm_rem, sizeof(arm_rem));
	}

	return rc ? -errno : rc;
}

static unsigned long sys_cacheflush(archsim::core::thread::ThreadInstance *cpu, uint32_t start, uint32_t end)
{
	cpu->GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, (void*)(uint64_t)0);

	return 0;
}

static unsigned long sys_dup(archsim::core::thread::ThreadInstance *cpu, int oldfd)
{
	int result = dup(oldfd);
	if(result) return result;
	else return -errno;
}

static unsigned long sys_readlinkat(archsim::core::thread::ThreadInstance *cpu, int dirfd, unsigned long long pathname, unsigned long long buf, size_t bufsiz)
{
	char buffer[256];
	cpu->GetMemoryInterface(0).ReadString(Address(pathname), buffer, 256);
	std::string requested_path = std::string(buffer);

	if(requested_path == "/proc/self/exe") {

		// HACK:
		std::string my_name_is = archsim::options::TargetBinary.GetValue();
		char *realname = realpath(my_name_is.c_str(), NULL);
		unsigned int sz = strlen(realname);
		cpu->GetMemoryInterface(0).Write(Address(buf), (unsigned char*)realname, sz);
		free(realname);
		return sz;
	} else {
		return -1;
	}
}


static unsigned long sys_readlink(archsim::core::thread::ThreadInstance *cpu, unsigned long pathname, unsigned long buf, unsigned long bufsize)
{
	return sys_readlinkat(cpu, AT_FDCWD, pathname, buf, bufsize);
}

static unsigned long sys_x86_arch_prctl(archsim::core::thread::ThreadInstance *thread, uint32_t code, uint64_t addr)
{
	switch(code) {
		case ARCH_SET_FS:
			*thread->GetRegisterFileInterface().GetEntry<uint64_t>("FS") = addr;
			break;
		case ARCH_SET_GS:
			*thread->GetRegisterFileInterface().GetEntry<uint64_t>("GS") = addr;
			break;

		case ARCH_GET_FS:
			thread->GetMemoryInterface(0).Write64(Address(addr), *thread->GetRegisterFileInterface().GetEntry<uint64_t>("FS"));
			break;
		case ARCH_GET_GS:
			thread->GetMemoryInterface(0).Write64(Address(addr), *thread->GetRegisterFileInterface().GetEntry<uint64_t>("GS"));
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static unsigned long sys_fchdir(archsim::core::thread::ThreadInstance *thread, unsigned int fd)
{
	int result = fchdir(translate_fd(thread, fd));
	if(result < 0) {
		return -errno;
	} else {
		return 0;
	}
}

static unsigned long sys_getdents_x86_64(archsim::core::thread::ThreadInstance *thread, unsigned int fd, unsigned long addr, unsigned int count)
{
	fd = translate_fd(thread, fd);

	// XXX This absolutely won't work if we're emulating 64-on-32 or 32-on-64

	std::vector<char> buffer(count);
	unsigned long result;
	asm volatile ("syscall" : "=a"(result) : "a"((unsigned long)78), "D"(fd), "S"(buffer.data()), "d"(count));

	if(result > 0) {
		thread->GetMemoryInterface(0).Write(Address(addr), (uint8_t*)buffer.data(), count);
	}

	return result;
}

static unsigned long sys_futex(archsim::core::thread::ThreadInstance *thread, unsigned long addr, unsigned int op, unsigned int val, unsigned long timespec_addr, unsigned long uaddr2, unsigned int val3)
{
	archsim::abi::memory::LockedMemoryRegion guest_region0, guest_region1;

	thread->GetEmulationModel().GetMemoryModel().LockRegions(Address(addr), 4, guest_region0);
	thread->GetEmulationModel().GetMemoryModel().LockRegions(Address(uaddr2), 4, guest_region1);

	void *ptr0 = guest_region0.GetPtr(Address(addr), 4);
	void *ptr1 = nullptr;
	if(uaddr2 != 0) {
		ptr1 = guest_region1.GetPtr(Address(uaddr2), 4);
	}

	struct timespec timeout;
	if(timespec_addr) {
		thread->GetEmulationModel().GetMemoryModel().Read(Address(timespec_addr), (uint8_t*)&timeout, 16);
	}

	unsigned long result = syscall(202, ptr0, op, val, &timeout, ptr1, val3);

	return result;
}

static unsigned long syscall_return_zero(archsim::core::thread::ThreadInstance* cpu)
{
	return 0;
}
static unsigned long syscall_return_enosys(archsim::core::thread::ThreadInstance* cpu)
{
	return -ENOSYS;
}

DEFINE_SYSCALL(arm, __NR_exit, sys_exit, "exit()");
DEFINE_SYSCALL(arm, __NR_exit_group, sys_exit, "exit_group()");

DEFINE_SYSCALL(arm, __NR_open, sys_open, "open(path=%p, mode=%d, flags=%d)");
DEFINE_SYSCALL(arm, __NR_openat, sys_openat, "open(dirfd=%u, path=%p, mode=%d, flags=%d)");
DEFINE_SYSCALL(arm, __NR_close, sys_close, "close(fd=%d)");
DEFINE_SYSCALL(arm, __NR_read, sys_read, "read(fd=%d, addr=%p, len=%d)");
DEFINE_SYSCALL(arm, __NR_write, sys_write, "write(fd=%d, addr=%p, len=%d)");
DEFINE_SYSCALL(arm, __NR_lseek, sys_lseek, "lseek(fd=%d, offset=%d, whence=%d)");
DEFINE_SYSCALL(arm, __NR__llseek, sys_llseek, "llseek(fd=%d, offset_high=%d, offset_low=%d, offset_result=%x, whence=%d)");
DEFINE_SYSCALL(arm, __NR_unlink, sys_unlink, "unlink(path=%p)");

DEFINE_SYSCALL(arm, __NR_dup, sys_dup, "dup(oldfd=%d)");

DEFINE_SYSCALL(arm, __NR_uname, sys_uname, "uname(addr=%p)");
DEFINE_SYSCALL(arm, __NR_writev, sys_writev<struct arm_iovec>, "writev()");
DEFINE_SYSCALL(arm, __NR_mmap, sys_mmap, "mmap()");
DEFINE_SYSCALL(arm, __NR_mmap2, sys_mmap2, "mmap2(addr=%p, size=%u, prot=%d, flags=%d, fd=%d, pgoff=%u)");
DEFINE_SYSCALL(arm, __NR_mremap, sys_mremap, "mremap()");
DEFINE_SYSCALL(arm, __NR_munmap, sys_munmap, "munmap(addr=%p, size=%d)");
DEFINE_SYSCALL(arm, __NR_mprotect, sys_mprotect, "mprotect(addr=%p, size=%d, prot=%d)");
DEFINE_SYSCALL(arm, __NR_fstat64, sys_fstat64, "fstat64(fd=%d, addr=%p)");
DEFINE_SYSCALL(arm, __NR_stat64, sys_stat64, "stat64(path=%p, addr=%p)");
DEFINE_SYSCALL(arm, __NR_lstat64, sys_lstat64, "lstat64(path=%p, addr=%p)");
DEFINE_SYSCALL(arm, __NR_ioctl, sys_ioctl, "ioctl(fd=%d, request=%d, ...)");
DEFINE_SYSCALL(arm, __NR_fcntl64, sys_fcntl64, "fcntl64(fd=%d, cmd=%d, ...)");

DEFINE_SYSCALL(arm, __NR_mkdir, sys_mkdir, "mkdir(path=%s, mode=%d)");
DEFINE_SYSCALL(arm, __NR_getcwd, sys_getcwd, "getcwd(path=%p, size=%d)");

DEFINE_SYSCALL(arm, __ARM_NR_set_tls, sys_arm_settls, "settls(addr=%p)");
DEFINE_SYSCALL(arm, __NR_brk, sys_brk, "brk(new_brk=%p)");

DEFINE_SYSCALL(arm, __NR_gettid, syscall_return_zero, "gettid()");
DEFINE_SYSCALL(arm, __NR_getpid, syscall_return_zero, "getpid()");
DEFINE_SYSCALL(arm, __NR_getuid, syscall_return_zero, "getuid()");
DEFINE_SYSCALL(arm, __NR_getgid, syscall_return_zero, "getgid()");
DEFINE_SYSCALL(arm, __NR_geteuid, syscall_return_zero, "geteuid()");
DEFINE_SYSCALL(arm, __NR_getegid, syscall_return_zero, "getegid()");
DEFINE_SYSCALL(arm, __NR_getuid32, syscall_return_zero, "getuid32()");
DEFINE_SYSCALL(arm, __NR_getgid32, syscall_return_zero, "getgid32()");
DEFINE_SYSCALL(arm, __NR_geteuid32, syscall_return_zero, "geteuid32()");
DEFINE_SYSCALL(arm, __NR_getegid32, syscall_return_zero, "getegid32()");

DEFINE_SYSCALL(arm, __NR_gettimeofday, sys_gettimeofday, "gettimeofday()");

DEFINE_SYSCALL(arm, __NR_rt_sigprocmask, sys_rt_sigprocmask, "rt_sigprocmask(how=%d, set=%p, old_set=%p)");
DEFINE_SYSCALL(arm, __NR_rt_sigaction, sys_rt_sigaction, "rt_sigaction(signum=%d, act=%p, old_act=%p)");
DEFINE_SYSCALL(arm, __NR_nanosleep, sys_nanosleep, "nanosleep(req=%p, rem=%p)");
DEFINE_SYSCALL(arm, __NR_times, sys_times, "times(buf=%p)");
DEFINE_SYSCALL(arm, __NR_clock_gettime, sys_clock_gettime, "clock_gettime(clk_id=%u, timespec=%p)");

DEFINE_SYSCALL(arm, __ARM_NR_cacheflush, sys_cacheflush, "cacheflush(%p, %p)");

/* x86-64 Syscalls */
DEFINE_SYSCALL(x86, 0, sys_read, "read()");
DEFINE_SYSCALL(x86, 1, sys_write, "write()");
DEFINE_SYSCALL(x86, 2, sys_open, "open()");
DEFINE_SYSCALL(x86, 3, sys_close, "close()");
DEFINE_SYSCALL(x86, 4, sys_stat64, "stat()");
DEFINE_SYSCALL(x86, 5, sys_fstat64, "fstat()");
DEFINE_SYSCALL(x86, 6, sys_lstat_x86, "lstat()");
DEFINE_SYSCALL(x86, 8, sys_lseek, "lseek()");
DEFINE_SYSCALL(x86, 9, sys_mmap, "mmap()");
DEFINE_SYSCALL(x86, 10, sys_mprotect, "mprotect()");
DEFINE_SYSCALL(x86, 12, sys_brk, "brk()");
DEFINE_SYSCALL(x86, 16, sys_ioctl, "ioctl()");
DEFINE_SYSCALL(x86, 20, sys_writev<struct x86_iovec>, "writev()");
DEFINE_SYSCALL(x86, 21, syscall_return_enosys, "access()");
DEFINE_SYSCALL(x86, 39, syscall_return_zero, "getpid()");
DEFINE_SYSCALL(x86, 63, sys_uname_x86, "uname()");
DEFINE_SYSCALL(x86, 72, sys_fcntl64, "fcntl()");
DEFINE_SYSCALL(x86, 78, sys_getdents_x86_64, "getdents()");
DEFINE_SYSCALL(x86, 79, sys_getcwd, "getcwd()");
DEFINE_SYSCALL(x86, 81, sys_fchdir, "fchdir()");
DEFINE_SYSCALL(x86, 83, sys_mkdir, "mkdir()");
DEFINE_SYSCALL(x86, 89, sys_readlink, "readlink()");
DEFINE_SYSCALL(x86, 96, sys_gettimeofday, "gettimeofday()");
DEFINE_SYSCALL(x86, 102, syscall_return_zero, "getuid()");
DEFINE_SYSCALL(x86, 104, syscall_return_zero, "getgid()");
DEFINE_SYSCALL(x86, 107, syscall_return_zero, "geteuid()");
DEFINE_SYSCALL(x86, 108, syscall_return_zero, "getegid()");
DEFINE_SYSCALL(x86, 158, sys_x86_arch_prctl, "arch_prctl()");
DEFINE_SYSCALL(x86, 186, syscall_return_zero, "gettid()");
DEFINE_SYSCALL(x86, 202, sys_futex, "futex()");
DEFINE_SYSCALL(x86, 228, sys_clock_gettime, "clock_gettime()");
DEFINE_SYSCALL(x86, 231, sys_exit, "exit_group()");
DEFINE_SYSCALL(x86, 257, sys_openat, "openat()");

/* Aarch64 System calls */
DEFINE_SYSCALL(aarch64, 1, sys_exit, "exit()");
DEFINE_SYSCALL(aarch64, 3, sys_read, "read()");
DEFINE_SYSCALL(aarch64, 4, sys_write, "write()");
DEFINE_SYSCALL(aarch64, 29, sys_ioctl, "ioctl()");
DEFINE_SYSCALL(aarch64, 56, sys_openat, "openat()");
DEFINE_SYSCALL(aarch64, 64, sys_write, "write(%u, %lu, %u)");
DEFINE_SYSCALL(aarch64, 66, sys_writev<struct x86_iovec>, "writev()");
DEFINE_SYSCALL(aarch64, 78, sys_readlinkat, "readlinkat()");
DEFINE_SYSCALL(aarch64, 80, sys64_fstat, "fstat()");
DEFINE_SYSCALL(aarch64, 160, sys_uname, "uname()");
DEFINE_SYSCALL(aarch64, 214, sys_brk, "brk()");
DEFINE_SYSCALL(aarch64, 222, sys_mmap2, "brk()");

/* RISC-V SYSTEM CALLS */
DEFINE_SYSCALL(riscv, 64, sys_write, "write(fd=%d, addr=%p, len=%d)");
DEFINE_SYSCALL(riscv, 160, sys_uname, "uname(addr=%p)");
DEFINE_SYSCALL(riscv, 214, sys_brk, "brk(new_brk=%p)");
DEFINE_SYSCALL(riscv, 78, syscall_return_enosys, "readlinkat(dirfd=%d, pathname=%p, buf=%p, bufsiz=%u)");
DEFINE_SYSCALL(riscv, 222, sys_mmap, "mmap()");
//DEFINE_SYSCALL("risc-v", 80, sys_fstat, "fstat()");
DEFINE_SYSCALL(riscv, 93, sys_exit, "exit()");
DEFINE_SYSCALL(riscv, 94, sys_exit, "exit_group()");
DEFINE_SYSCALL(riscv, 215, sys_munmap, "fstat()");


DEFINE_SYSCALL(riscv, 56, sys_openat, "openat(dirfd=%d, pathname=%p, flags=%d, mode=%d)");
DEFINE_SYSCALL(riscv, 23, syscall_return_enosys, "_unknown_()");
//DEFINE_SYSCALL("risc-v", 25, sys_fcntl, "fcntl()");
DEFINE_SYSCALL(riscv, 80, sys_fstat, "fstat()");
DEFINE_SYSCALL(riscv, 57, sys_close, "close()");
DEFINE_SYSCALL(riscv, 63, sys_read, "read()");
