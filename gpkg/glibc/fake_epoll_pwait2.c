#include <stddef.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sysdep-cancel.h>
#include <sys/syscall.h>

#define MSEC_PER_SEC 1000L
#define NSEC_PER_MSEC 1000000L

static __attribute__((unused)) int
fake_epoll_pwait2 (int epfd, struct epoll_event *events, int maxevents,
		   const struct __timespec64 *timeout, const sigset_t *sigmask, size_t sigsetsize) {
	long timeout_long = timeout->tv_sec * MSEC_PER_SEC;
	if (timeout->tv_nsec > 0) {
		timeout_long += timeout->tv_nsec / NSEC_PER_MSEC;
		if (timeout->tv_nsec % NSEC_PER_MSEC > 0)
			timeout_long += 1;
	}
	return SYSCALL_CANCEL (epoll_pwait, epfd, events, maxevents, timeout_long, sigmask, sigsetsize);
}
