#ifndef _FAKE_SYSCALL
#define _FAKE_SYSCALL

#include <arch-syscall.h>
#include <disabled-syscall.h>
#undef DISABLED_SYSCALL_WITH_FAKESYSCALL

#ifndef _UNISTD_H
extern long int syscall (long int __sysno, ...) __THROW;
#endif

#endif //_FAKE_SYSCALL
