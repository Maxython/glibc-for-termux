#ifndef _IGNORE_SYSCALL
#define _IGNORE_SYSCALL

#include <arch-syscall.h>

struct IdSyscalls {
	int id;
};

static struct IdSyscalls ListIgnoreSyscall[] = {
	{ __NR_mbind },
	{ 1008 }, // for some reason used in julia
};

#define count_ignore_syscall (sizeof(ListIgnoreSyscall) / sizeof(ListIgnoreSyscall[0]))

#endif //_IGNORE_SYSCALL
