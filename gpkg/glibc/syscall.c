#ifndef WITHOUT_FAKESYSCALL
# include <fakesyscall-base.h>
#else
# include <unistd.h>
#endif
#include <stdarg.h>

long int
syscall (long int number, ...)
{
  va_list args;
  va_start (args, number);
  long int a0 = va_arg (args, long int);
  long int a1 = va_arg (args, long int);
  long int a2 = va_arg (args, long int);
  long int a3 = va_arg (args, long int);
  long int a4 = va_arg (args, long int);
  long int a5 = va_arg (args, long int);
  va_end (args);

#ifndef WITHOUT_FAKESYSCALL
  switch (number)
#endif
  {
#ifndef WITHOUT_FAKESYSCALL
    DISABLED_SYSCALL_WITH_FAKESYSCALL
    default:
#endif
      return syscallS (number, a0, a1, a2, a3, a4, a5);
  }
}
