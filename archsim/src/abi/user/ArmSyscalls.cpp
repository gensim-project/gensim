/*
 * abi/user/ArmSyscalls.cpp
 */
#include "system.h"

#include "abi/user/SyscallHandler.h"
#include "abi/user/ArmSyscalls.h"
#include "abi/EmulationModel.h"
#include "abi/UserEmulationModel.h"
#include "abi/memory/MemoryModel.h"

#include "gensim/gensim_processor.h"

#include "util/SimOptions.h"
#include "util/LogContext.h"

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

UseLogContext(LogSyscalls);

static unsigned int translate_fd(gensim::Processor& cpu, int fd)
{
	fd = cpu.GetEmulationModel().GetSystem().GetFD(fd);

	return fd;
}

static unsigned int sys_exit(gensim::Processor& cpu, unsigned int exit_code)
{
	cpu.Halt();
	cpu.GetEmulationModel().GetSystem().exit_code = exit_code;

	return 0;
}

static unsigned int sys_uname(gensim::Processor& cpu, unsigned int addr)
{
	if (addr == 0) {
		return -EFAULT;
	}

	cpu.GetMemoryModel().WriteString(addr, "Linux");

	addr += 64;
	cpu.GetMemoryModel().WriteString(addr, "archsim");

	addr += 64;
	cpu.GetMemoryModel().WriteString(addr, "3.6.3");

	addr += 64;
	cpu.GetMemoryModel().WriteString(addr, "#1");

	addr += 64;
	cpu.GetMemoryModel().WriteString(addr, "arm");

	return 0;
}

static unsigned int sys_open(gensim::Processor& cpu, unsigned int filename_addr, unsigned int flags, unsigned int mode)
{
	char filename[256];
	cpu.GetMemoryModel().ReadString(filename_addr, filename, sizeof(filename) - 1);

	int host_fd = open(filename, flags, mode);
	if (host_fd < 0) return -errno;

	int guest_fd = cpu.GetEmulationModel().GetSystem().OpenFD(host_fd);

	return guest_fd;
}

static unsigned int sys_openat(gensim::Processor& cpu, int dirfd, unsigned int filename_addr, unsigned int flags, unsigned int mode)
{
	if(dirfd == AT_FDCWD) {
		return sys_open(cpu, filename_addr, flags, mode);
	}

	char filename[256];
	cpu.GetMemoryModel().ReadString(filename_addr, filename, sizeof(filename) - 1);

	int host_fd = openat(translate_fd(cpu, dirfd),filename, flags, mode);
	if (host_fd < 0) return -errno;

	int guest_fd = cpu.GetEmulationModel().GetSystem().OpenFD(host_fd);

	return guest_fd;
}

static unsigned int sys_close(gensim::Processor& cpu, unsigned int fd)
{
	if (cpu.GetEmulationModel().GetSystem().CloseFD(fd))
		return -errno;
	return 0;
}

static unsigned int sys_writev(gensim::Processor& cpu, unsigned int fd, unsigned int iov_addr, int cnt)
{
	if (cnt < 0 || cnt > 8) return -1;

	struct iovec host_vectors[cnt];
	struct arm_iovec arm_vectors[cnt];

	cpu.GetMemoryModel().Read(iov_addr, (uint8_t *)&arm_vectors, cnt * sizeof(struct arm_iovec));

	for (int i = 0; i < cnt; i++) {
		host_vectors[i].iov_base = new char[arm_vectors[i].iov_len];
		host_vectors[i].iov_len = arm_vectors[i].iov_len;
		cpu.GetMemoryModel().Read(arm_vectors[i].iov_base, (uint8_t *)host_vectors[i].iov_base, arm_vectors[i].iov_len);
	}

	fd = cpu.GetEmulationModel().GetSystem().GetFD(fd);
	fsync(fd);  // HACK?
	unsigned int res = (uint32_t)writev(fd, host_vectors, cnt);
	fsync(fd);  // HACK?

	for (int i = 0; i < cnt; i++) delete[](char*)host_vectors[i].iov_base;

	return res;
}

static unsigned int sys_lseek(gensim::Processor& cpu, unsigned int fd, unsigned int offset, unsigned int whence)
{
	fd = translate_fd(cpu, fd);
	int lseek_result = lseek(fd, offset, whence);
	if (lseek_result < 0)
		return -errno;
	else
		return lseek_result;
}

static unsigned int sys_llseek(gensim::Processor& cpu, unsigned int fd, unsigned int offset_high, unsigned int offset_low, unsigned int result_addr, unsigned int whence)
{
	loff_t result;

	fd = translate_fd(cpu, fd);
	uint64_t offset;
	if(archsim::options::ArmOabi)
		offset = ((((uint64_t)offset_low) << 32) | (uint64_t)offset_high);
	else
		offset = ((((uint64_t)offset_high) << 32) | (uint64_t)offset_low);

	result = lseek64(fd, offset, whence);

	if ((result != -1) && result_addr) {
		cpu.GetMemoryModel().Write64(result_addr, (uint64_t)result);
	}

	return result == -1 ? -errno : 0;
}

static unsigned int sys_unlink(gensim::Processor& cpu, unsigned int filename_addr)
{
	char filename[256];
	cpu.GetMemoryModel().ReadString(filename_addr, filename, sizeof(filename) - 1);

	if (unlink(filename)) return -errno;

	return 0;
}

static unsigned int sys_mmap(gensim::Processor& cpu, unsigned int addr, unsigned int len, unsigned int prot, unsigned int flags, unsigned int fd, unsigned int off)
{
	if (!(flags & MAP_ANONYMOUS)) {
		LC_ERROR(LogSyscalls) << "Unsupported usage of MMAP";
		return -EINVAL;
	}

	if (fd != (unsigned int)-1) {
		LC_ERROR(LogSyscalls) << "Attempted to MMAP a file";
		return -EINVAL;
	}

	if (!cpu.GetMemoryModel().IsAligned(len)) {
		return -EINVAL;
	}

	archsim::abi::memory::RegionFlags reg_flags = (archsim::abi::memory::RegionFlags)(archsim::abi::memory::RegFlagRead | archsim::abi::memory::RegFlagWrite);
	if(prot & PROT_EXEC) reg_flags = (archsim::abi::memory::RegionFlags)(reg_flags | archsim::abi::memory::RegFlagExecute);

	if (addr == 0 && !(flags & MAP_FIXED)) {
		addr = cpu.GetMemoryModel().GetMappingManager()->MapAnonymousRegion(len, reg_flags);
	} else {
		if (!cpu.GetMemoryModel().GetMappingManager()->MapRegion(addr, len, reg_flags, ""))
			return -1;
	}

	return addr;
}

static unsigned int sys_mmap2(gensim::Processor& cpu, unsigned int addr, unsigned int len, unsigned int prot, unsigned int flags, unsigned int fd, unsigned int off)
{
	return sys_mmap(cpu, addr, len, prot, flags, fd, off);
}

static unsigned int sys_mremap(gensim::Processor& cpu, unsigned int old_addr, unsigned int old_size, unsigned int new_size, unsigned int flags)
{
	LC_ERROR(LogSyscalls) << "mremap not supported";
	return -1;
}

static unsigned int sys_munmap(gensim::Processor& cpu, unsigned int addr, unsigned int len)
{
	if (!cpu.GetMemoryModel().IsAligned(len)) {
		return -EINVAL;
	}

	cpu.GetMemoryModel().GetMappingManager()->UnmapRegion(addr, len);
	return 0;
}

static unsigned int sys_mprotect(gensim::Processor &cpu, unsigned int addr, unsigned int len, unsigned int flags)
{
	LC_ERROR(LogSyscalls) << "[SYSCALL] mprotect not currently supported";
	LC_ERROR(LogSyscalls) << "[SYSCALL] " << std::hex << addr << " " << len << " " << flags;
	return -EINVAL;
}

static unsigned int sys_read(gensim::Processor& cpu, unsigned int fd, unsigned int addr, unsigned int len)
{
	char* rd_buf = new char[len];
	ssize_t res;

	fd = translate_fd(cpu, fd);
	res = read(fd, (void*)rd_buf, len);

	if (res > 0) {
		cpu.GetMemoryModel().Write(addr, (uint8_t *)rd_buf, res);
	}

	delete[] rd_buf;

	if (res < 0)
		return -errno;
	else
		return res;
}

static unsigned int sys_write(gensim::Processor& cpu, unsigned int fd, unsigned int addr, unsigned int len)
{
	ssize_t res;

	char *buffer = (char *)malloc(len);
	bzero(buffer, len);

	if (cpu.GetMemoryModel().Read(addr, (uint8_t *)buffer, len)) {
		free(buffer);
		return -EFAULT;
	}

	fd = translate_fd(cpu, fd);
	res = write(fd, buffer, len);
	free(buffer);

	if (res < 0)
		return -errno;
	else
		return res;
}

static unsigned int sys_fstat64(gensim::Processor& cpu, unsigned int fd, unsigned int addr)
{
	struct stat64 st;
	struct arm_stat64 result_st;

	fd = translate_fd(cpu, fd);
	if (fstat64(fd, &st)) {
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

	cpu.GetMemoryModel().Write(addr, (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned int sys_fstat(gensim::Processor& cpu, unsigned int fd, unsigned int addr)
{
	struct stat st;
	struct arm_stat result_st;

	fd = translate_fd(cpu, fd);
	if (fstat(fd, &st)) {
		return -errno;
	}

	memset(&result_st, 0, sizeof(result_st));

	result_st.st_dev = st.st_dev;
	result_st.__st_ino = st.st_ino;
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

	cpu.GetMemoryModel().Write(addr, (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned int sys_stat64(gensim::Processor& cpu, unsigned int filename_addr, unsigned int addr)
{
	struct stat64 st;
	struct arm_stat64 result_st;

	char filename[256];
	cpu.GetMemoryModel().ReadString(filename_addr, filename, sizeof(filename) - 1);

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

	cpu.GetMemoryModel().Write(addr, (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned int sys_lstat64(gensim::Processor& cpu, unsigned int filename_addr, unsigned int addr)
{
	struct stat64 st;
	struct arm_stat64 result_st;

	char filename[256];
	cpu.GetMemoryModel().ReadString(filename_addr, filename, sizeof(filename) - 1);

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

	cpu.GetMemoryModel().Write(addr, (uint8_t *)&result_st, sizeof(result_st));

	return 0;
}

static unsigned int sys_ioctl(gensim::Processor& cpu, unsigned int fd, unsigned int request, unsigned int a0)
{
	fd = translate_fd(cpu, fd);

	switch (request) {
		case 0x5401: {
			struct termio host;
			unsigned int rc = ioctl(fd, request, &host);

			if (rc) {
				return -errno;
			}

			cpu.GetMemoryModel().Write(a0, (uint8_t *)&host, sizeof(host));
			return 0;
		}

		default:
			fprintf(stderr, "ioctl: %d NOT IMPLEMENTED\n", request);
			return -ENOSYS;
	}
}

static unsigned int sys_fcntl64(gensim::Processor& cpu, unsigned int fd, unsigned int cmd, unsigned int a0)
{
	int rc = fcntl(translate_fd(cpu, fd), cmd, a0);
	if (rc) return -errno;
	return 0;
}

static unsigned int sys_mkdir(gensim::Processor& cpu, unsigned int filename_addr, unsigned int mode)
{
	char filename[256];
	cpu.GetMemoryModel().ReadString(filename_addr, filename, sizeof(filename) - 1);

	int rc = mkdir(filename, mode);
	if (rc) return -errno;

	return 0;
}

static unsigned int sys_getcwd(gensim::Processor& cpu, unsigned int buffer_addr, unsigned int size)
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
	cpu.GetMemoryModel().WriteString(buffer_addr, cwd);
	free(cwd);

	return 0;
}

static unsigned int sys_arm_settls(gensim::Processor& cpu, unsigned int addr)
{
	cpu.GetMemoryModel().Poke(0xffff0ff0, (uint8_t *)&addr, sizeof(addr));
	return 0;
}

static unsigned int sys_brk(gensim::Processor& cpu, unsigned int new_brk)
{
	archsim::abi::UserEmulationModel& uem = static_cast<archsim::abi::UserEmulationModel&>(cpu.GetEmulationModel());
	auto oldbrk = uem.GetBreak();
	if (new_brk >= uem.GetInitialBreak()) uem.SetBreak(new_brk);

	LC_DEBUG1(LogSyscalls) << "BRK: old=" << std::hex << oldbrk << ", requested=" << std::hex << new_brk << ", new=" << std::hex << uem.GetBreak();
	return uem.GetBreak();
}

static unsigned int sys_gettimeofday(gensim::Processor& cpu, unsigned int tv_addr, unsigned int tz_addr)
{
	struct timeval host_tv;
	struct timezone host_tz;

	struct arm_timeval guest_tv;

	if (tv_addr == 0) {
		return -EFAULT;
	}

	if(archsim::options::Verify) {
		guest_tv.tv_sec = (uint32_t)0;
		guest_tv.tv_usec = (uint32_t)0;

		cpu.GetMemoryModel().Write(tv_addr, (uint8_t *)&guest_tv, sizeof(guest_tv));
		return 0;
	}

	if (gettimeofday(&host_tv, &host_tz)) {
		return -errno;
	}

	guest_tv.tv_sec = (uint32_t)host_tv.tv_sec;
	guest_tv.tv_usec = (uint32_t)host_tv.tv_usec;

	cpu.GetMemoryModel().Write(tv_addr, (uint8_t *)&guest_tv, sizeof(guest_tv));

	return 0;
}

static unsigned int sys_rt_sigprocmask(gensim::Processor& cpu, unsigned int how, unsigned int set_ptr, unsigned int oldset_ptr)
{
	return 0;
}

static unsigned int sys_rt_sigaction(gensim::Processor& cpu, unsigned int signum, unsigned int act_ptr, unsigned int oldact_ptr)
{
	if(!archsim::options::UserPermitSignalHandling) return -EINVAL;
	if (act_ptr == 0) {
		if (oldact_ptr == 0) {
			return -EINVAL;
		} else {
			archsim::abi::SignalData *data;
			if (!cpu.GetEmulationModel().GetSignalData(signum, data))
				return 0;

			cpu.GetMemoryModel().Write(oldact_ptr, (uint8_t *)data->priv, sizeof(struct arm_sigaction));
			return 0;
		}
	} else {
		archsim::abi::SignalData *data = NULL;
		void *act = NULL;
		if (!cpu.GetEmulationModel().GetSignalData(signum, data)) {
			act = malloc(sizeof(struct arm_sigaction));
			if (act == NULL)
				return -EFAULT;
		} else {
			assert(data);
			act = data->priv;
		}

		cpu.GetMemoryModel().Read(act_ptr, (uint8_t *)act, sizeof(struct arm_sigaction));
		cpu.GetEmulationModel().CaptureSignal(signum, ((struct arm_sigaction *)act)->sa_handler, act);
	}

	return 0;
}

static unsigned int sys_times(gensim::Processor& cpu, unsigned int buf_addr)
{
	if (buf_addr == 0)
		return -EFAULT;

	struct tms host_buf;
	struct arm_tms guest_buf;

	if(archsim::options::Verify) {
		guest_buf.tms_utime = (uint32_t)0;
		guest_buf.tms_stime = (uint32_t)0;
		guest_buf.tms_cutime = (uint32_t)0;
		guest_buf.tms_cstime = (uint32_t)0;

		cpu.GetMemoryModel().Write(buf_addr, (uint8_t *)&guest_buf, sizeof(guest_buf));

		return 0;
	}

	if (times(&host_buf))
		return -errno;

	guest_buf.tms_utime = (uint32_t)host_buf.tms_utime;
	guest_buf.tms_stime = (uint32_t)host_buf.tms_stime;
	guest_buf.tms_cutime = (uint32_t)host_buf.tms_cutime;
	guest_buf.tms_cstime = (uint32_t)host_buf.tms_cstime;

	cpu.GetMemoryModel().Write(buf_addr, (uint8_t *)&guest_buf, sizeof(guest_buf));

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

static unsigned int sys_clock_gettime(gensim::Processor &cpu, unsigned int clk_id, unsigned int timespec_ptr)
{
	struct arm_timespec arm_ts;
	struct timespec ts;

	if(archsim::options::Verify) {
		arm_ts.tv_nsec = 0;
		arm_ts.tv_sec = 0;
		cpu.GetMemoryModel().Write(timespec_ptr, (uint8_t*)&arm_ts, sizeof(arm_ts));
		return 0;
	}

	int result = clock_gettime(clk_id, &ts);
	host_timespec_to_arm(&ts, &arm_ts);

	cpu.GetMemoryModel().Write(timespec_ptr, (uint8_t*)&arm_ts, sizeof(arm_ts));

	return -result;
}

static unsigned int sys_nanosleep(gensim::Processor& cpu, unsigned int req_ptr, unsigned int rem_ptr)
{
	struct arm_timespec arm_req, arm_rem;
	struct timespec req, rem;
	int rc;

	bzero(&req, sizeof(req));
	bzero(&rem, sizeof(rem));
	bzero(&arm_req, sizeof(arm_req));
	bzero(&arm_rem, sizeof(arm_rem));

	cpu.GetMemoryModel().Read(req_ptr, (uint8_t *)&arm_req, sizeof(arm_req));
	arm_timespec_to_host(&arm_req, &req);

	rc = nanosleep(&req, &rem);

	if (rem_ptr != 0) {
		host_timespec_to_arm(&rem, &arm_rem);
		cpu.GetMemoryModel().Write(rem_ptr, (uint8_t *)&arm_rem, sizeof(arm_rem));
	}

	return rc ? -errno : rc;
}

static unsigned int sys_cacheflush(gensim::Processor &cpu, uint32_t start, uint32_t end)
{
	cpu.GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, (void*)(uint64_t)0);

	return 0;
}

static unsigned int sys_dup(gensim::Processor &cpu, int oldfd)
{
	int result = dup(oldfd);
	if(result) return result;
	else return -errno;
}

static unsigned int syscall_return_zero(gensim::Processor& cpu)
{
	return 0;
}
static unsigned int syscall_return_enosys(gensim::Processor& cpu)
{
	return -ENOSYS;
}

DEFINE_SYSCALL("arm", __NR_exit, sys_exit, "exit()");
DEFINE_SYSCALL("arm", __NR_exit_group, sys_exit, "exit_group()");

DEFINE_SYSCALL("arm", __NR_open, sys_open, "open(path=%p, mode=%d, flags=%d)");
DEFINE_SYSCALL("arm", __NR_close, sys_close, "close(fd=%d)");
DEFINE_SYSCALL("arm", __NR_read, sys_read, "read(fd=%d, addr=%p, len=%d)");
DEFINE_SYSCALL("arm", __NR_write, sys_write, "write(fd=%d, addr=%p, len=%d)");
DEFINE_SYSCALL("arm", __NR_lseek, sys_lseek, "lseek(fd=%d, offset=%d, whence=%d)");
DEFINE_SYSCALL("arm", __NR__llseek, sys_llseek, "llseek(fd=%d, offset_high=%d, offset_low=%d, offset_result=%x, whence=%d)");
DEFINE_SYSCALL("arm", __NR_unlink, sys_unlink, "unlink(path=%p)");

DEFINE_SYSCALL("arm", __NR_dup, sys_dup, "dup(oldfd=%d)");

DEFINE_SYSCALL("arm", __NR_uname, sys_uname, "uname(addr=%p)");
DEFINE_SYSCALL("arm", __NR_writev, sys_writev, "writev()");
DEFINE_SYSCALL("arm", __NR_mmap, sys_mmap, "mmap()");
DEFINE_SYSCALL("arm", __NR_mmap2, sys_mmap2, "mmap2(addr=%p, size=%u, prot=%d, flags=%d, fd=%d, pgoff=%u)");
DEFINE_SYSCALL("arm", __NR_mremap, sys_mremap, "mremap()");
DEFINE_SYSCALL("arm", __NR_munmap, sys_munmap, "munmap(addr=%p, size=%d)");
DEFINE_SYSCALL("arm", __NR_mprotect, sys_mprotect, "mprotect(addr=%p, size=%d, prot=%d)");
DEFINE_SYSCALL("arm", __NR_fstat64, sys_fstat64, "fstat64(fd=%d, addr=%p)");
DEFINE_SYSCALL("arm", __NR_stat64, sys_stat64, "stat64(path=%p, addr=%p)");
DEFINE_SYSCALL("arm", __NR_lstat64, sys_lstat64, "lstat64(path=%p, addr=%p)");
DEFINE_SYSCALL("arm", __NR_ioctl, sys_ioctl, "ioctl(fd=%d, request=%d, ...)");
DEFINE_SYSCALL("arm", __NR_fcntl64, sys_fcntl64, "fcntl64(fd=%d, cmd=%d, ...)");

DEFINE_SYSCALL("arm", __NR_mkdir, sys_mkdir, "mkdir(path=%s, mode=%d)");
DEFINE_SYSCALL("arm", __NR_getcwd, sys_getcwd, "getcwd(path=%p, size=%d)");

DEFINE_SYSCALL("arm", __ARM_NR_set_tls, sys_arm_settls, "settls(addr=%p)");
DEFINE_SYSCALL("arm", __NR_brk, sys_brk, "brk(new_brk=%p)");

DEFINE_SYSCALL("arm", __NR_gettid, syscall_return_zero, "gettid()");
DEFINE_SYSCALL("arm", __NR_getpid, syscall_return_zero, "getpid()");
DEFINE_SYSCALL("arm", __NR_getuid, syscall_return_zero, "getuid()");
DEFINE_SYSCALL("arm", __NR_getgid, syscall_return_zero, "getgid()");
DEFINE_SYSCALL("arm", __NR_geteuid, syscall_return_zero, "geteuid()");
DEFINE_SYSCALL("arm", __NR_getegid, syscall_return_zero, "getegid()");
DEFINE_SYSCALL("arm", __NR_getuid32, syscall_return_zero, "getuid32()");
DEFINE_SYSCALL("arm", __NR_getgid32, syscall_return_zero, "getgid32()");
DEFINE_SYSCALL("arm", __NR_geteuid32, syscall_return_zero, "geteuid32()");
DEFINE_SYSCALL("arm", __NR_getegid32, syscall_return_zero, "getegid32()");

DEFINE_SYSCALL("arm", __NR_gettimeofday, sys_gettimeofday, "gettimeofday()");

DEFINE_SYSCALL("arm", __NR_rt_sigprocmask, sys_rt_sigprocmask, "rt_sigprocmask(how=%d, set=%p, old_set=%p)");
DEFINE_SYSCALL("arm", __NR_rt_sigaction, sys_rt_sigaction, "rt_sigaction(signum=%d, act=%p, old_act=%p)");
DEFINE_SYSCALL("arm", __NR_nanosleep, sys_nanosleep, "nanosleep(req=%p, rem=%p)");
DEFINE_SYSCALL("arm", __NR_times, sys_times, "times(buf=%p)");
DEFINE_SYSCALL("arm", __NR_clock_gettime, sys_clock_gettime, "clock_gettime(clk_id=%u, timespec=%p)");

DEFINE_SYSCALL("arm", __ARM_NR_cacheflush, sys_cacheflush, "cacheflush(%p, %p)");


/* RISC-V SYSTEM CALLS */
DEFINE_SYSCALL("risc-v", 64, sys_write, "write(fd=%d, addr=%p, len=%d)");
DEFINE_SYSCALL("risc-v", 160, sys_uname, "uname(addr=%p)");
DEFINE_SYSCALL("risc-v", 214, sys_brk, "brk(new_brk=%p)");
DEFINE_SYSCALL("risc-v", 78, syscall_return_enosys, "readlinkat(dirfd=%d, pathname=%p, buf=%p, bufsiz=%u)");
DEFINE_SYSCALL("risc-v", 222, sys_mmap, "mmap()");
//DEFINE_SYSCALL("risc-v", 80, sys_fstat, "fstat()");
DEFINE_SYSCALL("risc-v", 93, sys_exit, "exit()");
DEFINE_SYSCALL("risc-v", 94, sys_exit, "exit_group()");
DEFINE_SYSCALL("risc-v", 215, sys_munmap, "fstat()");


DEFINE_SYSCALL("risc-v", 56, sys_openat, "openat(dirfd=%d, pathname=%p, flags=%d, mode=%d)");
DEFINE_SYSCALL("risc-v", 23, syscall_return_enosys, "_unknown_()");
//DEFINE_SYSCALL("risc-v", 25, sys_fcntl, "fcntl()");
DEFINE_SYSCALL("risc-v", 80, sys_fstat, "fstat()");
DEFINE_SYSCALL("risc-v", 57, sys_close, "close()");
DEFINE_SYSCALL("risc-v", 63, sys_read, "read()");
