#ifndef _FAKE_SYSCALL_BASE
#define _FAKE_SYSCALL_BASE

#include <sysdep.h>
#include <arch-syscall.h>
#include <disabled-syscall.h>
#include <fcntl.h>

// close_range
// fchownat
// ftruncate
// getpgrp
// unlinkat
// symlink
// link
// faccessat
#include <unistd.h>

// accept4
// recvfrom
// sendto
#include <sys/socket.h>

// fchmodat
#include <sys/stat.h>

// clock_gettime
#include <time.h>

// shmat
// shmctl
// shmdt
// shmget
#include <sys/shm.h>

// statx_generic
#include "io/statx_generic.c"

// fake_epoll_pwait2
#include "fake_epoll_pwait2.c"

#endif //_FAKE_SYSCALL_BASE
