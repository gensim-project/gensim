/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   ArmSyscalls.h
 * Author: s0457958
 *
 * Created on 20 November 2013, 10:51
 */

#ifndef ARMSYSCALLS_H
#define ARMSYSCALLS_H

#include <sys/stat.h>

#define __NR_SYSCALL_BASE 0

#define __NR_arm_restart_syscall (__NR_SYSCALL_BASE + 0)
#define __NR_arm_exit (__NR_SYSCALL_BASE + 1)
#define __NR_arm_fork (__NR_SYSCALL_BASE + 2)
#define __NR_arm_read (__NR_SYSCALL_BASE + 3)
#define __NR_arm_write (__NR_SYSCALL_BASE + 4)
#define __NR_arm_open (__NR_SYSCALL_BASE + 5)
#define __NR_arm_close (__NR_SYSCALL_BASE + 6)
/* 7 was sys_waitpid */
#define __NR_arm_creat (__NR_SYSCALL_BASE + 8)
#define __NR_arm_link (__NR_SYSCALL_BASE + 9)
#define __NR_arm_unlink (__NR_SYSCALL_BASE + 10)
#define __NR_arm_execve (__NR_SYSCALL_BASE + 11)
#define __NR_arm_chdir (__NR_SYSCALL_BASE + 12)
#define __NR_arm_time (__NR_SYSCALL_BASE + 13)
#define __NR_arm_mknod (__NR_SYSCALL_BASE + 14)
#define __NR_arm_chmod (__NR_SYSCALL_BASE + 15)
#define __NR_arm_lchown (__NR_SYSCALL_BASE + 16)
/* 17 was sys_break */
/* 18 was sys_stat */
#define __NR_arm_lseek (__NR_SYSCALL_BASE + 19)
#define __NR_arm_getpid (__NR_SYSCALL_BASE + 20)
#define __NR_arm_mount (__NR_SYSCALL_BASE + 21)
#define __NR_arm_umount (__NR_SYSCALL_BASE + 22)
#define __NR_arm_setuid (__NR_SYSCALL_BASE + 23)
#define __NR_arm_getuid (__NR_SYSCALL_BASE + 24)
#define __NR_arm_stime (__NR_SYSCALL_BASE + 25)
#define __NR_arm_ptrace (__NR_SYSCALL_BASE + 26)
#define __NR_arm_alarm (__NR_SYSCALL_BASE + 27)
/* 28 was sys_fstat */
#define __NR_arm_pause (__NR_SYSCALL_BASE + 29)
#define __NR_arm_utime (__NR_SYSCALL_BASE + 30)
/* 31 was sys_stty */
/* 32 was sys_gtty */
#define __NR_arm_access (__NR_SYSCALL_BASE + 33)
#define __NR_arm_nice (__NR_SYSCALL_BASE + 34)
/* 35 was sys_ftime */
#define __NR_arm_sync (__NR_SYSCALL_BASE + 36)
#define __NR_arm_kill (__NR_SYSCALL_BASE + 37)
#define __NR_arm_rename (__NR_SYSCALL_BASE + 38)
#define __NR_arm_mkdir (__NR_SYSCALL_BASE + 39)
#define __NR_arm_rmdir (__NR_SYSCALL_BASE + 40)
#define __NR_arm_dup (__NR_SYSCALL_BASE + 41)
#define __NR_arm_pipe (__NR_SYSCALL_BASE + 42)
#define __NR_arm_times (__NR_SYSCALL_BASE + 43)
/* 44 was sys_prof */
#define __NR_arm_brk (__NR_SYSCALL_BASE + 45)
#define __NR_arm_setgid (__NR_SYSCALL_BASE + 46)
#define __NR_arm_getgid (__NR_SYSCALL_BASE + 47)
/* 48 was sys_signal */
#define __NR_arm_geteuid (__NR_SYSCALL_BASE + 49)
#define __NR_arm_getegid (__NR_SYSCALL_BASE + 50)
#define __NR_arm_acct (__NR_SYSCALL_BASE + 51)
#define __NR_arm_umount2 (__NR_SYSCALL_BASE + 52)
/* 53 was sys_lock */
#define __NR_arm_ioctl (__NR_SYSCALL_BASE + 54)
#define __NR_arm_fcntl (__NR_SYSCALL_BASE + 55)
/* 56 was sys_mpx */
#define __NR_arm_setpgid (__NR_SYSCALL_BASE + 57)
/* 58 was sys_ulimit */
/* 59 was sys_olduname */
#define __NR_arm_umask (__NR_SYSCALL_BASE + 60)
#define __NR_arm_chroot (__NR_SYSCALL_BASE + 61)
#define __NR_arm_ustat (__NR_SYSCALL_BASE + 62)
#define __NR_arm_dup2 (__NR_SYSCALL_BASE + 63)
#define __NR_arm_getppid (__NR_SYSCALL_BASE + 64)
#define __NR_arm_getpgrp (__NR_SYSCALL_BASE + 65)
#define __NR_arm_setsid (__NR_SYSCALL_BASE + 66)
#define __NR_arm_sigaction (__NR_SYSCALL_BASE + 67)
/* 68 was sys_sgetmask */
/* 69 was sys_ssetmask */
#define __NR_arm_setreuid (__NR_SYSCALL_BASE + 70)
#define __NR_arm_setregid (__NR_SYSCALL_BASE + 71)
#define __NR_arm_sigsuspend (__NR_SYSCALL_BASE + 72)
#define __NR_arm_sigpending (__NR_SYSCALL_BASE + 73)
#define __NR_arm_sethostname (__NR_SYSCALL_BASE + 74)
#define __NR_arm_setrlimit (__NR_SYSCALL_BASE + 75)
#define __NR_arm_getrlimit (__NR_SYSCALL_BASE + 76) /* Back compat 2GB limited rlimit */
#define __NR_arm_getrusage (__NR_SYSCALL_BASE + 77)
#define __NR_arm_gettimeofday (__NR_SYSCALL_BASE + 78)
#define __NR_arm_settimeofday (__NR_SYSCALL_BASE + 79)
#define __NR_arm_getgroups (__NR_SYSCALL_BASE + 80)
#define __NR_arm_setgroups (__NR_SYSCALL_BASE + 81)
#define __NR_arm_select (__NR_SYSCALL_BASE + 82)
#define __NR_arm_symlink (__NR_SYSCALL_BASE + 83)
/* 84 was sys_lstat */
#define __NR_arm_readlink (__NR_SYSCALL_BASE + 85)
#define __NR_arm_uselib (__NR_SYSCALL_BASE + 86)
#define __NR_arm_swapon (__NR_SYSCALL_BASE + 87)
#define __NR_arm_reboot (__NR_SYSCALL_BASE + 88)
#define __NR_arm_readdir (__NR_SYSCALL_BASE + 89)
#define __NR_arm_mmap (__NR_SYSCALL_BASE + 90)
#define __NR_arm_munmap (__NR_SYSCALL_BASE + 91)
#define __NR_arm_truncate (__NR_SYSCALL_BASE + 92)
#define __NR_arm_ftruncate (__NR_SYSCALL_BASE + 93)
#define __NR_arm_fchmod (__NR_SYSCALL_BASE + 94)
#define __NR_arm_fchown (__NR_SYSCALL_BASE + 95)
#define __NR_arm_getpriority (__NR_SYSCALL_BASE + 96)
#define __NR_arm_setpriority (__NR_SYSCALL_BASE + 97)
/* 98 was sys_profil */
#define __NR_arm_statfs (__NR_SYSCALL_BASE + 99)
#define __NR_arm_fstatfs (__NR_SYSCALL_BASE + 100)
/* 101 was sys_ioperm */
#define __NR_arm_socketcall (__NR_SYSCALL_BASE + 102)
#define __NR_arm_syslog (__NR_SYSCALL_BASE + 103)
#define __NR_arm_setitimer (__NR_SYSCALL_BASE + 104)
#define __NR_arm_getitimer (__NR_SYSCALL_BASE + 105)
#define __NR_arm_stat (__NR_SYSCALL_BASE + 106)
#define __NR_arm_lstat (__NR_SYSCALL_BASE + 107)
#define __NR_arm_fstat (__NR_SYSCALL_BASE + 108)
/* 109 was sys_uname */
/* 110 was sys_iopl */
#define __NR_arm_vhangup (__NR_SYSCALL_BASE + 111)
/* 112 was sys_idle */
#define __NR_arm_syscall (__NR_SYSCALL_BASE + 113) /* syscall to call a syscall! */
#define __NR_arm_wait4 (__NR_SYSCALL_BASE + 114)
#define __NR_arm_swapoff (__NR_SYSCALL_BASE + 115)
#define __NR_arm_sysinfo (__NR_SYSCALL_BASE + 116)
#define __NR_arm_ipc (__NR_SYSCALL_BASE + 117)
#define __NR_arm_fsync (__NR_SYSCALL_BASE + 118)
#define __NR_arm_sigreturn (__NR_SYSCALL_BASE + 119)
#define __NR_arm_clone (__NR_SYSCALL_BASE + 120)
#define __NR_arm_setdomainname (__NR_SYSCALL_BASE + 121)
#define __NR_arm_uname (__NR_SYSCALL_BASE + 122)
/* 123 was sys_modify_ldt */
#define __NR_arm_adjtimex (__NR_SYSCALL_BASE + 124)
#define __NR_arm_mprotect (__NR_SYSCALL_BASE + 125)
#define __NR_arm_sigprocmask (__NR_SYSCALL_BASE + 126)
/* 127 was sys_create_module */
#define __NR_arm_init_module (__NR_SYSCALL_BASE + 128)
#define __NR_arm_delete_module (__NR_SYSCALL_BASE + 129)
/* 130 was sys_get_kernel_syms */
#define __NR_arm_quotactl (__NR_SYSCALL_BASE + 131)
#define __NR_arm_getpgid (__NR_SYSCALL_BASE + 132)
#define __NR_arm_fchdir (__NR_SYSCALL_BASE + 133)
#define __NR_arm_bdflush (__NR_SYSCALL_BASE + 134)
#define __NR_arm_sysfs (__NR_SYSCALL_BASE + 135)
#define __NR_arm_personality (__NR_SYSCALL_BASE + 136)
/* 137 was sys_afs_syscall */
#define __NR_arm_setfsuid (__NR_SYSCALL_BASE + 138)
#define __NR_arm_setfsgid (__NR_SYSCALL_BASE + 139)
#define __NR_arm__llseek (__NR_SYSCALL_BASE + 140)
#define __NR_arm_getdents (__NR_SYSCALL_BASE + 141)
#define __NR_arm__newselect (__NR_SYSCALL_BASE + 142)
#define __NR_arm_flock (__NR_SYSCALL_BASE + 143)
#define __NR_arm_msync (__NR_SYSCALL_BASE + 144)
#define __NR_arm_readv (__NR_SYSCALL_BASE + 145)
#define __NR_arm_writev (__NR_SYSCALL_BASE + 146)
#define __NR_arm_getsid (__NR_SYSCALL_BASE + 147)
#define __NR_arm_fdatasync (__NR_SYSCALL_BASE + 148)
#define __NR_arm__sysctl (__NR_SYSCALL_BASE + 149)
#define __NR_arm_mlock (__NR_SYSCALL_BASE + 150)
#define __NR_arm_munlock (__NR_SYSCALL_BASE + 151)
#define __NR_arm_mlockall (__NR_SYSCALL_BASE + 152)
#define __NR_arm_munlockall (__NR_SYSCALL_BASE + 153)
#define __NR_arm_sched_setparam (__NR_SYSCALL_BASE + 154)
#define __NR_arm_sched_getparam (__NR_SYSCALL_BASE + 155)
#define __NR_arm_sched_setscheduler (__NR_SYSCALL_BASE + 156)
#define __NR_arm_sched_getscheduler (__NR_SYSCALL_BASE + 157)
#define __NR_arm_sched_yield (__NR_SYSCALL_BASE + 158)
#define __NR_arm_sched_get_priority_max (__NR_SYSCALL_BASE + 159)
#define __NR_arm_sched_get_priority_min (__NR_SYSCALL_BASE + 160)
#define __NR_arm_sched_rr_get_interval (__NR_SYSCALL_BASE + 161)
#define __NR_arm_nanosleep (__NR_SYSCALL_BASE + 162)
#define __NR_arm_mremap (__NR_SYSCALL_BASE + 163)
#define __NR_arm_setresuid (__NR_SYSCALL_BASE + 164)
#define __NR_arm_getresuid (__NR_SYSCALL_BASE + 165)
/* 166 was sys_vm86 */
/* 167 was sys_query_module */
#define __NR_arm_poll (__NR_SYSCALL_BASE + 168)
#define __NR_arm_nfsservctl (__NR_SYSCALL_BASE + 169)
#define __NR_arm_setresgid (__NR_SYSCALL_BASE + 170)
#define __NR_arm_getresgid (__NR_SYSCALL_BASE + 171)
#define __NR_arm_prctl (__NR_SYSCALL_BASE + 172)
#define __NR_arm_rt_sigreturn (__NR_SYSCALL_BASE + 173)
#define __NR_arm_rt_sigaction (__NR_SYSCALL_BASE + 174)
#define __NR_arm_rt_sigprocmask (__NR_SYSCALL_BASE + 175)
#define __NR_arm_rt_sigpending (__NR_SYSCALL_BASE + 176)
#define __NR_arm_rt_sigtimedwait (__NR_SYSCALL_BASE + 177)
#define __NR_arm_rt_sigqueueinfo (__NR_SYSCALL_BASE + 178)
#define __NR_arm_rt_sigsuspend (__NR_SYSCALL_BASE + 179)
#define __NR_arm_pread64 (__NR_SYSCALL_BASE + 180)
#define __NR_arm_pwrite64 (__NR_SYSCALL_BASE + 181)
#define __NR_arm_chown (__NR_SYSCALL_BASE + 182)
#define __NR_arm_getcwd (__NR_SYSCALL_BASE + 183)
#define __NR_arm_capget (__NR_SYSCALL_BASE + 184)
#define __NR_arm_capset (__NR_SYSCALL_BASE + 185)
#define __NR_arm_sigaltstack (__NR_SYSCALL_BASE + 186)
#define __NR_arm_sendfile (__NR_SYSCALL_BASE + 187)
/* 188 reserved */
/* 189 reserved */
#define __NR_arm_vfork (__NR_SYSCALL_BASE + 190)
#define __NR_arm_ugetrlimit (__NR_SYSCALL_BASE + 191) /* SuS compliant getrlimit */
#define __NR_arm_mmap2 (__NR_SYSCALL_BASE + 192)
#define __NR_arm_truncate64 (__NR_SYSCALL_BASE + 193)
#define __NR_arm_ftruncate64 (__NR_SYSCALL_BASE + 194)
#define __NR_arm_stat64 (__NR_SYSCALL_BASE + 195)
#define __NR_arm_lstat64 (__NR_SYSCALL_BASE + 196)
#define __NR_arm_fstat64 (__NR_SYSCALL_BASE + 197)
#define __NR_arm_lchown32 (__NR_SYSCALL_BASE + 198)
#define __NR_arm_getuid32 (__NR_SYSCALL_BASE + 199)
#define __NR_arm_getgid32 (__NR_SYSCALL_BASE + 200)
#define __NR_arm_geteuid32 (__NR_SYSCALL_BASE + 201)
#define __NR_arm_getegid32 (__NR_SYSCALL_BASE + 202)
#define __NR_arm_setreuid32 (__NR_SYSCALL_BASE + 203)
#define __NR_arm_setregid32 (__NR_SYSCALL_BASE + 204)
#define __NR_arm_getgroups32 (__NR_SYSCALL_BASE + 205)
#define __NR_arm_setgroups32 (__NR_SYSCALL_BASE + 206)
#define __NR_arm_fchown32 (__NR_SYSCALL_BASE + 207)
#define __NR_arm_setresuid32 (__NR_SYSCALL_BASE + 208)
#define __NR_arm_getresuid32 (__NR_SYSCALL_BASE + 209)
#define __NR_arm_setresgid32 (__NR_SYSCALL_BASE + 210)
#define __NR_arm_getresgid32 (__NR_SYSCALL_BASE + 211)
#define __NR_arm_chown32 (__NR_SYSCALL_BASE + 212)
#define __NR_arm_setuid32 (__NR_SYSCALL_BASE + 213)
#define __NR_arm_setgid32 (__NR_SYSCALL_BASE + 214)
#define __NR_arm_setfsuid32 (__NR_SYSCALL_BASE + 215)
#define __NR_arm_setfsgid32 (__NR_SYSCALL_BASE + 216)
#define __NR_arm_getdents64 (__NR_SYSCALL_BASE + 217)
#define __NR_arm_pivot_root (__NR_SYSCALL_BASE + 218)
#define __NR_arm_mincore (__NR_SYSCALL_BASE + 219)
#define __NR_arm_madvise (__NR_SYSCALL_BASE + 220)
#define __NR_arm_fcntl64 (__NR_SYSCALL_BASE + 221)
/* 222 for tux */
/* 223 is unused */
#define __NR_arm_gettid (__NR_SYSCALL_BASE + 224)
#define __NR_arm_readahead (__NR_SYSCALL_BASE + 225)
#define __NR_arm_setxattr (__NR_SYSCALL_BASE + 226)
#define __NR_arm_lsetxattr (__NR_SYSCALL_BASE + 227)
#define __NR_arm_fsetxattr (__NR_SYSCALL_BASE + 228)
#define __NR_arm_getxattr (__NR_SYSCALL_BASE + 229)
#define __NR_arm_lgetxattr (__NR_SYSCALL_BASE + 230)
#define __NR_arm_fgetxattr (__NR_SYSCALL_BASE + 231)
#define __NR_arm_listxattr (__NR_SYSCALL_BASE + 232)
#define __NR_arm_llistxattr (__NR_SYSCALL_BASE + 233)
#define __NR_arm_flistxattr (__NR_SYSCALL_BASE + 234)
#define __NR_arm_removexattr (__NR_SYSCALL_BASE + 235)
#define __NR_arm_lremovexattr (__NR_SYSCALL_BASE + 236)
#define __NR_arm_fremovexattr (__NR_SYSCALL_BASE + 237)
#define __NR_arm_tkill (__NR_SYSCALL_BASE + 238)
#define __NR_arm_sendfile64 (__NR_SYSCALL_BASE + 239)
#define __NR_arm_futex (__NR_SYSCALL_BASE + 240)
#define __NR_arm_sched_setaffinity (__NR_SYSCALL_BASE + 241)
#define __NR_arm_sched_getaffinity (__NR_SYSCALL_BASE + 242)
#define __NR_arm_io_setup (__NR_SYSCALL_BASE + 243)
#define __NR_arm_io_destroy (__NR_SYSCALL_BASE + 244)
#define __NR_arm_io_getevents (__NR_SYSCALL_BASE + 245)
#define __NR_arm_io_submit (__NR_SYSCALL_BASE + 246)
#define __NR_arm_io_cancel (__NR_SYSCALL_BASE + 247)
#define __NR_arm_exit_group (__NR_SYSCALL_BASE + 248)
#define __NR_arm_lookup_dcookie (__NR_SYSCALL_BASE + 249)
#define __NR_arm_epoll_create (__NR_SYSCALL_BASE + 250)
#define __NR_arm_epoll_ctl (__NR_SYSCALL_BASE + 251)
#define __NR_arm_epoll_wait (__NR_SYSCALL_BASE + 252)
#define __NR_arm_remap_file_pages (__NR_SYSCALL_BASE + 253)
/* 254 for set_thread_area */
/* 255 for get_thread_area */
#define __NR_arm_set_tid_address (__NR_SYSCALL_BASE + 256)
#define __NR_arm_timer_create (__NR_SYSCALL_BASE + 257)
#define __NR_arm_timer_settime (__NR_SYSCALL_BASE + 258)
#define __NR_arm_timer_gettime (__NR_SYSCALL_BASE + 259)
#define __NR_arm_timer_getoverrun (__NR_SYSCALL_BASE + 260)
#define __NR_arm_timer_delete (__NR_SYSCALL_BASE + 261)
#define __NR_arm_clock_settime (__NR_SYSCALL_BASE + 262)
#define __NR_arm_clock_gettime (__NR_SYSCALL_BASE + 263)
#define __NR_arm_clock_getres (__NR_SYSCALL_BASE + 264)
#define __NR_arm_clock_nanosleep (__NR_SYSCALL_BASE + 265)
#define __NR_arm_statfs64 (__NR_SYSCALL_BASE + 266)
#define __NR_arm_fstatfs64 (__NR_SYSCALL_BASE + 267)
#define __NR_arm_tgkill (__NR_SYSCALL_BASE + 268)
#define __NR_arm_utimes (__NR_SYSCALL_BASE + 269)
#define __NR_arm_arm_fadvise64_64 (__NR_SYSCALL_BASE + 270)
#define __NR_arm_pciconfig_iobase (__NR_SYSCALL_BASE + 271)
#define __NR_arm_pciconfig_read (__NR_SYSCALL_BASE + 272)
#define __NR_arm_pciconfig_write (__NR_SYSCALL_BASE + 273)
#define __NR_arm_mq_open (__NR_SYSCALL_BASE + 274)
#define __NR_arm_mq_unlink (__NR_SYSCALL_BASE + 275)
#define __NR_arm_mq_timedsend (__NR_SYSCALL_BASE + 276)
#define __NR_arm_mq_timedreceive (__NR_SYSCALL_BASE + 277)
#define __NR_arm_mq_notify (__NR_SYSCALL_BASE + 278)
#define __NR_arm_mq_getsetattr (__NR_SYSCALL_BASE + 279)
#define __NR_arm_waitid (__NR_SYSCALL_BASE + 280)
#define __NR_arm_socket (__NR_SYSCALL_BASE + 281)
#define __NR_arm_bind (__NR_SYSCALL_BASE + 282)
#define __NR_arm_connect (__NR_SYSCALL_BASE + 283)
#define __NR_arm_listen (__NR_SYSCALL_BASE + 284)
#define __NR_arm_accept (__NR_SYSCALL_BASE + 285)
#define __NR_arm_getsockname (__NR_SYSCALL_BASE + 286)
#define __NR_arm_getpeername (__NR_SYSCALL_BASE + 287)
#define __NR_arm_socketpair (__NR_SYSCALL_BASE + 288)
#define __NR_arm_send (__NR_SYSCALL_BASE + 289)
#define __NR_arm_sendto (__NR_SYSCALL_BASE + 290)
#define __NR_arm_recv (__NR_SYSCALL_BASE + 291)
#define __NR_arm_recvfrom (__NR_SYSCALL_BASE + 292)
#define __NR_arm_shutdown (__NR_SYSCALL_BASE + 293)
#define __NR_arm_setsockopt (__NR_SYSCALL_BASE + 294)
#define __NR_arm_getsockopt (__NR_SYSCALL_BASE + 295)
#define __NR_arm_sendmsg (__NR_SYSCALL_BASE + 296)
#define __NR_arm_recvmsg (__NR_SYSCALL_BASE + 297)
#define __NR_arm_semop (__NR_SYSCALL_BASE + 298)
#define __NR_arm_semget (__NR_SYSCALL_BASE + 299)
#define __NR_arm_semctl (__NR_SYSCALL_BASE + 300)
#define __NR_arm_msgsnd (__NR_SYSCALL_BASE + 301)
#define __NR_arm_msgrcv (__NR_SYSCALL_BASE + 302)
#define __NR_arm_msgget (__NR_SYSCALL_BASE + 303)
#define __NR_arm_msgctl (__NR_SYSCALL_BASE + 304)
#define __NR_arm_shmat (__NR_SYSCALL_BASE + 305)
#define __NR_arm_shmdt (__NR_SYSCALL_BASE + 306)
#define __NR_arm_shmget (__NR_SYSCALL_BASE + 307)
#define __NR_arm_shmctl (__NR_SYSCALL_BASE + 308)
#define __NR_arm_add_key (__NR_SYSCALL_BASE + 309)
#define __NR_arm_request_key (__NR_SYSCALL_BASE + 310)
#define __NR_arm_keyctl (__NR_SYSCALL_BASE + 311)
#define __NR_arm_semtimedop (__NR_SYSCALL_BASE + 312)
#define __NR_arm_vserver (__NR_SYSCALL_BASE + 313)
#define __NR_arm_ioprio_set (__NR_SYSCALL_BASE + 314)
#define __NR_arm_ioprio_get (__NR_SYSCALL_BASE + 315)
#define __NR_arm_inotify_init (__NR_SYSCALL_BASE + 316)
#define __NR_arm_inotify_add_watch (__NR_SYSCALL_BASE + 317)
#define __NR_arm_inotify_rm_watch (__NR_SYSCALL_BASE + 318)
#define __NR_arm_mbind (__NR_SYSCALL_BASE + 319)
#define __NR_arm_get_mempolicy (__NR_SYSCALL_BASE + 320)
#define __NR_arm_set_mempolicy (__NR_SYSCALL_BASE + 321)
#define __NR_arm_openat (__NR_SYSCALL_BASE + 322)
#define __NR_arm_mkdirat (__NR_SYSCALL_BASE + 323)
#define __NR_arm_mknodat (__NR_SYSCALL_BASE + 324)
#define __NR_arm_fchownat (__NR_SYSCALL_BASE + 325)
#define __NR_arm_futimesat (__NR_SYSCALL_BASE + 326)
#define __NR_arm_fstatat64 (__NR_SYSCALL_BASE + 327)
#define __NR_arm_unlinkat (__NR_SYSCALL_BASE + 328)
#define __NR_arm_renameat (__NR_SYSCALL_BASE + 329)
#define __NR_arm_linkat (__NR_SYSCALL_BASE + 330)
#define __NR_arm_symlinkat (__NR_SYSCALL_BASE + 331)
#define __NR_arm_readlinkat (__NR_SYSCALL_BASE + 332)
#define __NR_arm_fchmodat (__NR_SYSCALL_BASE + 333)
#define __NR_arm_faccessat (__NR_SYSCALL_BASE + 334)
#define __NR_arm_pselect6 (__NR_SYSCALL_BASE + 335)
#define __NR_arm_ppoll (__NR_SYSCALL_BASE + 336)
#define __NR_arm_unshare (__NR_SYSCALL_BASE + 337)
#define __NR_arm_set_robust_list (__NR_SYSCALL_BASE + 338)
#define __NR_arm_get_robust_list (__NR_SYSCALL_BASE + 339)
#define __NR_arm_splice (__NR_SYSCALL_BASE + 340)
#define __NR_arm_arm_sync_file_range (__NR_SYSCALL_BASE + 341)
#define __NR_arm_sync_file_range2 __NR_arm_sync_file_range
#define __NR_arm_tee (__NR_SYSCALL_BASE + 342)
#define __NR_arm_vmsplice (__NR_SYSCALL_BASE + 343)
#define __NR_arm_move_pages (__NR_SYSCALL_BASE + 344)
#define __NR_arm_getcpu (__NR_SYSCALL_BASE + 345)
#define __NR_arm_epoll_pwait (__NR_SYSCALL_BASE + 346)
#define __NR_arm_kexec_load (__NR_SYSCALL_BASE + 347)
#define __NR_arm_utimensat (__NR_SYSCALL_BASE + 348)
#define __NR_arm_signalfd (__NR_SYSCALL_BASE + 349)
#define __NR_arm_timerfd_create (__NR_SYSCALL_BASE + 350)
#define __NR_arm_eventfd (__NR_SYSCALL_BASE + 351)
#define __NR_arm_fallocate (__NR_SYSCALL_BASE + 352)
#define __NR_arm_timerfd_settime (__NR_SYSCALL_BASE + 353)
#define __NR_arm_timerfd_gettime (__NR_SYSCALL_BASE + 354)
#define __NR_arm_signalfd4 (__NR_SYSCALL_BASE + 355)
#define __NR_arm_eventfd2 (__NR_SYSCALL_BASE + 356)
#define __NR_arm_epoll_create1 (__NR_SYSCALL_BASE + 357)
#define __NR_arm_dup3 (__NR_SYSCALL_BASE + 358)
#define __NR_arm_pipe2 (__NR_SYSCALL_BASE + 359)
#define __NR_arm_inotify_init1 (__NR_SYSCALL_BASE + 360)
#define __NR_arm_preadv (__NR_SYSCALL_BASE + 361)
#define __NR_arm_pwritev (__NR_SYSCALL_BASE + 362)
#define __NR_arm_rt_tgsigqueueinfo (__NR_SYSCALL_BASE + 363)
#define __NR_arm_perf_event_open (__NR_SYSCALL_BASE + 364)

/*
 * The following SWIs are ARM private.
 */
#define __ARM_NR_BASE (__NR_SYSCALL_BASE + 0x0f0000)
#define __ARM_NR_breakpoint (__ARM_NR_BASE + 1)
#define __ARM_NR_cacheflush (__ARM_NR_BASE + 2)
#define __ARM_NR_usr26 (__ARM_NR_BASE + 3)
#define __ARM_NR_usr32 (__ARM_NR_BASE + 4)
#define __ARM_NR_set_tls (__ARM_NR_BASE + 5)

typedef unsigned int abi_ulong;

struct arm_iovec {
	abi_ulong iov_base;
	unsigned int iov_len;
} __attribute__((packed));

struct x86_iovec {
	uint64_t iov_base;
	uint64_t iov_len;
} __attribute__((packed));

struct riscv32_iovec {
	uint32_t iov_base;
	uint32_t iov_len;
} __attribute__((packed));

struct arm_stat64 {
	uint64_t st_dev;
	uint64_t __st_ino;
	uint64_t st_nlink;
	uint32_t st_mode;

	uint32_t st_uid;
	uint32_t st_gid;

	uint32_t pad0;

	uint64_t st_rdev;
	long long st_size;


	abi_ulong st_blksize;
	unsigned long long st_blocks;

	abi_ulong target_st_atime;
	abi_ulong target_st_atime_nsec;

	abi_ulong target_st_mtime;
	abi_ulong target_st_mtime_nsec;

	abi_ulong target_st_ctime;
	abi_ulong target_st_ctime_nsec;

	unsigned long long st_ino;
} __attribute__((packed));

struct x86_64_stat64 {
	dev_t st_dev;
	ino64_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off64_t st_size;
	timespec st_atim;
	timespec st_mtim;
	timespec st_ctim;
	blksize_t st_blksize;
	blkcnt64_t st_blocks;
	mode_t st_attr;
} __attribute__((packed));

using x86_64_stat = struct stat;

struct arm_stat {
	uint32_t st_dev;
	uint32_t __st_ino;
	uint16_t st_mode;
	uint16_t st_nlink;

	uint16_t st_uid;
	uint16_t st_gid;

	uint32_t st_rdev;

	uint32_t st_size;
	uint32_t st_blksize;
	uint32_t st_blocks;

	uint32_t target_st_atime;
	uint32_t target_st_atime_nsec;

	uint32_t target_st_mtime;
	uint32_t target_st_mtime_nsec;

	uint32_t target_st_ctime;
	uint32_t target_st_ctime_nsec;
} __attribute__((packed));

struct aarch64_stat {
	uint64_t st_dev;
	uint64_t __st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	uint64_t st_rdev;
	uint64_t padding;
	int64_t st_size;
	int32_t st_blksize;
	int32_t padding2;
	int64_t st_blocks;
	int64_t target_st_atime;
	uint64_t st_atime_nsec;
	int64_t target_st_mtime;
	uint64_t st_mtime_nsec;
	int64_t target_st_ctime;
	uint64_t st_ctime_nsec;
	uint32_t padding3;
	uint32_t padding4;
} __attribute__((packed));

struct arm_timeval {
	unsigned int tv_sec;
	unsigned int tv_usec;
} __attribute__((packed));

struct arm_timespec {
	unsigned int tv_sec;
	unsigned int tv_nsec;
};

#define _ARM_SIGSET_NWORDS (1024 / (8 * 4))

struct arm_sigset {
	uint32_t __val[_ARM_SIGSET_NWORDS];
};

struct arm_sigaction {
	uint32_t sa_handler;
	struct arm_sigset sa_mask;
	int sa_flags;
	uint32_t sa_restorer;
};

struct arm_tms {
	uint32_t tms_utime;  /* clock_t */
	uint32_t tms_stime;  /* clock_t */
	uint32_t tms_cutime; /* clock_t */
	uint32_t tms_cstime; /* clock_t */
};

struct riscv32_stat {
	using dev_t = uint64_t;
	using ino_t = uint64_t;
	using mode_t = uint32_t;
	using nlink_t = uint32_t;
	using uid_t = uint32_t;
	using gid_t = uint32_t;
	using off_t = uint64_t;
	using blksize_t = uint32_t;
	using blkcnt_t = uint64_t;
	using timespec = struct {
		uint32_t tv_sec, tv_nsec;
	};

	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	unsigned long __pad;
	off_t st_size;
	blksize_t st_blksize;
	int __pad2;
	blkcnt_t st_blocks;
	timespec st_atim;
	timespec st_mtim;
	timespec st_ctim;
	unsigned __unused[2];
};

template<typename host_stat, typename guest_stat> static void translate_stat(const host_stat &in, guest_stat &out)
{
	out.st_dev = in.st_dev;
	out.st_ino = in.st_ino;
	out.st_mode = in.st_mode;
	out.st_nlink = in.st_nlink;
	out.st_uid = in.st_uid;
	out.st_gid = in.st_gid;
	out.st_rdev = in.st_rdev;
	out.st_size = in.st_size;
	out.st_blksize = in.st_blksize;
	out.st_blocks = in.st_blocks;

	out.st_atim.tv_nsec = in.st_atim.tv_nsec;
	out.st_atim.tv_sec = in.st_atim.tv_sec;

	out.st_mtim.tv_sec = in.st_mtim.tv_sec;
	out.st_mtim.tv_nsec = in.st_mtim.tv_nsec;

	out.st_ctim.tv_nsec = in.st_ctim.tv_nsec;
	out.st_ctim.tv_sec = in.st_ctim.tv_sec;
}

#endif /* ARMSYSCALLS_H */
