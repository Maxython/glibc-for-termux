/* - Fix for Termux
   If you run mprotect with the PROT_EXEC flag,
   then the error "Permission denied" is displayed.

   Cause: https://github.com/termux/proot/commit/89bfa991cb3cb7fc78099d06d0f7e7c840cb62d1
   Issue: https://github.com/termux-pacman/glibc-packages/issues/49
   Solution: https://stackoverflow.com/questions/59303617/c-mprotect-for-read-write-and-execute
*/

#include <errno.h>
#include <sys/mman.h>
#include <sysdep.h>
#include <sys/syscall.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <not-cancel.h>

#ifdef SHARED
# define GLOBAL_READ_SIZE 1024

/* - String To Unsigned Long Int
   There are two chairs - one is called "strtoul", the other "stuli"...
*/
static unsigned long int __stuli(char *value) {
	unsigned long int val, res = 0;
	int len = strlen(value);

	for (int i=0; i<len; i++) {
		val = value[len-1-i]-'0';
		if (val == 0)
			continue;
		else if (val > 9)
			val = value[len-1-i]-'W';
		for (int j=0; j<i; j++)
			val *= 16;
		res += val;
	}

	return res;
}

static int __is_mmaped(void *addr) {
	char buff[GLOBAL_READ_SIZE];
	char *buff2 = NULL;
	char *cont = NULL;
	int res = 0;
	int strlc, strlb;
	memset(buff, 0, sizeof(buff));
	int map = __open_nocancel("/proc/self/maps", O_RDONLY|O_CLOEXEC);
	if (map >= 0) {
		while (__read_nocancel(map, buff, GLOBAL_READ_SIZE) > 0) {
			strlc = cont ? strlen(cont) : 0;
			if (strlc > 0) {
				buff2 = malloc(sizeof(char)*strlc);
				memcpy(buff2, cont, strlc);
			}
			cont = calloc(strlc+strlen(buff), sizeof(char));
			strlb = buff2 ? strlen(buff2): 0;
			if (strlb > 0)
				memcpy(cont, buff2, strlb);
			__strncat(cont, buff, GLOBAL_READ_SIZE);
			memset(buff, 0, sizeof(buff));
		}
		char *saveptr;
		while ((buff2 = strtok_r(cont, "\n", &cont)))
			if (__stuli(strtok_r(buff2, "-", &saveptr)) == (unsigned long int)addr) {
				res = 1;
				break;
		}
	}
	__close_nocancel_nostatus(map);
	return res;
}
#endif

int __mprotect(void *addr, size_t len, int prot) {
	int res = INLINE_SYSCALL_CALL(mprotect, addr, len, prot);
#ifdef SHARED
	if (res == -1 && errno == EACCES && prot & PROT_EXEC && !__is_mmaped(addr)) {
		size_t saddr = strlen(addr)+1;
		void *caddr;
		int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED;
		if (prot & PROT_GROWSDOWN)
			mmap_flags |= MAP_GROWSDOWN;
		if (saddr > 1) {
			caddr = malloc(saddr);
			memcpy(caddr, addr, saddr);
		}
		addr = mmap(addr, len, PROT_READ|PROT_WRITE|PROT_EXEC, mmap_flags, -1, 0);
		if (saddr > 1)
			memcpy(addr, caddr, saddr);
		return INLINE_SYSCALL_CALL(mprotect, addr, len, prot);
	}
#endif
	return res;
}

libc_hidden_def(__mprotect)
weak_alias(__mprotect, mprotect)
