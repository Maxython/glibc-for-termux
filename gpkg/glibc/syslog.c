/*
 * Refactored syslog.c that can work with the Android
 * log system (/dev/socket/logdw). It consists of functions
 * that were taken from the following bionic scripts:
 * - syslog.cpp (https://cs.android.com/android/platform/superproject/main/+/main:bionic/libc/bionic/syslog.cpp)
 * - async_safe_log.cpp (https://cs.android.com/android/platform/superproject/main/+/main:bionic/libc/async_safe/async_safe_log.cpp)
 *
 * Addition: in the future it is planned to implement
 * two types of syslog: one will work with android (bionic
 * implementation), the other will work with termux+systemd
 * environment (glibc implementation).
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <time.h>
#include <linux/un.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <libio/libioP.h>

#define ANDROID_LOG_VERBOSE 2 // not used :/
#define ANDROID_LOG_DEBUG 3
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_WARN 5
#define ANDROID_LOG_ERROR 6
#define ANDROID_LOG_FATAL 7

#define LOG_ID_CRASH 4
#define LOG_ID_MAIN 0

static const char* syslog_log_tag = NULL;
static int syslog_priority_mask = 0xff;
static int syslog_options = 0;
extern char *__progname;

typedef struct log_time {
	uint32_t tv_sec;
	uint32_t tv_nsec;
} __attribute__((__packed__)) log_time;

struct BufferOutputStream {
	size_t total;
	char* pos_;
	size_t avail_;
};

struct BufferOutputStream BufferOutput(char* buffer, size_t size) {
	struct BufferOutputStream buff;
	buff.total = 0;
	buff.pos_ = buffer;
	buff.avail_ = size;
	if (buff.avail_ > 0)
		buff.pos_[0] = '\0';
	return buff;
}

void Send(struct BufferOutputStream* buff, const char* data, int len) {
	if (len < 0)
		len = strlen(data);
	buff->total += len;

	if (buff->avail_ <= 1)
		// No space to put anything else.
		return;

	if ((size_t)len >= buff->avail_)
		len = buff->avail_ - 1;
	memcpy(buff->pos_, data, len);
	buff->pos_ += len;
	buff->pos_[0] = '\0';
	buff->avail_ -= len;
}

static unsigned parse_decimal(const char* format, int* ppos) {
	const char* p = format + *ppos;
	unsigned result = 0;

	for (;;) {
		int ch = *p;
		unsigned d = (unsigned)(ch - '0');

		if (d >= 10U)
			break;

		result = result * 10 + d;
		p++;
	}
	*ppos = p - format;
	return result;
}

static void format_unsigned(char* buf, size_t buf_size, uint64_t value, int base, bool caps) {
	char* p = buf;
	char* end = buf + buf_size - 1;

	// Generate digit string in reverse order.
	while (value) {
		unsigned d = value % base;
		value /= base;
		if (p != end) {
			char ch;
			if (d < 10)
				ch = '0' + d;
			else
				ch = (caps ? 'A' : 'a') + (d - 10);
			*p++ = ch;
		}
	}

	// Special case for 0.
	if (p == buf)
		if (p != end)
			*p++ = '0';
	*p = '\0';

	// Reverse digit string in-place.
	size_t length = p - buf;
	for (size_t i = 0, j = length - 1; i < j; ++i, --j) {
		char ch = buf[i];
		buf[i] = buf[j];
		buf[j] = ch;
	}
}

static void format_integer(char* buf, size_t buf_size, uint64_t value, char conversion) {
	// Decode the conversion specifier.
	int is_signed = (conversion == 'd' || conversion == 'i' || conversion == 'o');
	int base = 10;
	if (tolower(conversion) == 'x')
		base = 16;
	else if (conversion == 'o')
		base = 8;
	else if (tolower(conversion) == 'b')
		base = 2;
	bool caps = (conversion == 'X');

	if (is_signed && (int64_t)value < 0) {
		buf[0] = '-';
		buf += 1;
		buf_size -= 1;
		value = (uint64_t)(-(int64_t)value);
	}
	format_unsigned(buf, buf_size, value, base, caps);
}

static void SendRepeat(struct BufferOutputStream* buff, char ch, int count) {
	char pad[8];
	memset(pad, ch, sizeof(pad));

	const int pad_size = (int)sizeof(pad);
	while (count > 0) {
		int avail = count;
		if (avail > pad_size)
			avail = pad_size;
		Send(buff, pad, avail);
		count -= avail;
	}
}

static void out_vformat(struct BufferOutputStream* buff, const char* format, va_list args) {
	int nn = 0;

	for (;;) {
		int mm;
		int padZero = 0;
		int padLeft = 0;
		char sign = '\0';
		int width = -1;
		int prec = -1;
		bool alternate = false;
		size_t bytelen = sizeof(int);
		int slen;
		char buffer[64];	// temporary buffer used to format numbers/format errno string

		char c;

		/* first, find all characters that are not 0 or '%' */
		/* then send them to the output directly */
		mm = nn;
		do {
			c = format[mm];
			if (c == '\0' || c == '%') break;
			mm++;
		} while (1);

		if (mm > nn) {
			Send(buff, format + nn, mm - nn);
			nn = mm;
		}

		/* is this it ? then exit */
		if (c == '\0') break;

		/* nope, we are at a '%' modifier */
		nn++;	// skip it

		/* parse flags */
		for (;;) {
			c = format[nn++];
			if (c == '\0') { /* single trailing '%' ? */
				c = '%';
				Send(buff, &c, 1);
				return;
			} else if (c == '0') {
				padZero = 1;
				continue;
			} else if (c == '-') {
				padLeft = 1;
				continue;
			} else if (c == ' ' || c == '+') {
				sign = c;
				continue;
			} else if (c == '#') {
				alternate = true;
				continue;
			}
			break;
		}

		/* parse field width */
		if ((c >= '0' && c <= '9')) {
			nn--;
			width = (int)parse_decimal(format, &nn);
			c = format[nn++];
		}

		/* parse precision */
		if (c == '.') {
			prec = (int)parse_decimal(format, &nn);
			c = format[nn++];
		}

		/* length modifier */
		switch (c) {
			case 'h':
				bytelen = sizeof(short);
				if (format[nn] == 'h') {
					bytelen = sizeof(char);
					nn += 1;
				}
				c = format[nn++];
				break;
			case 'l':
				bytelen = sizeof(long);
				if (format[nn] == 'l') {
					bytelen = sizeof(long long);
					nn += 1;
				}
				c = format[nn++];
				break;
			case 'z':
				bytelen = sizeof(size_t);
				c = format[nn++];
				break;
			case 't':
				bytelen = sizeof(ptrdiff_t);
				c = format[nn++];
				break;
			default:;
		}

		/* conversion specifier */
		const char* str = buffer;
		if (c == 's') {
			/* string */
			str = va_arg(args, const char*);
		} else if (c == 'c') {
			/* character */
			/* NOTE: char is promoted to int when passed through the stack */
			buffer[0] = (char)va_arg(args, int);
			buffer[1] = '\0';
		} else if (c == 'p') {
			uint64_t value = (uintptr_t)va_arg(args, void*);
			buffer[0] = '0';
			buffer[1] = 'x';
			format_integer(buffer + 2, sizeof(buffer) - 2, value, 'x');
		} else if (c == 'm') {
			{
				strerror_r(errno, buffer, sizeof(buffer));
			}
		} else if (tolower(c) == 'b' || c == 'd' || c == 'i' || c == 'o' || c == 'u' ||
							 tolower(c) == 'x') {
			/* integers - first read value from stack */
			uint64_t value;
			int is_signed = (c == 'd' || c == 'i' || c == 'o');

			/* NOTE: int8_t and int16_t are promoted to int when passed
			 *			 through the stack
			 */
			switch (bytelen) {
				case 1:
					value = (uint8_t)va_arg(args, int);
					break;
				case 2:
					value = (uint16_t)va_arg(args, int);
					break;
				case 4:
					value = va_arg(args, uint32_t);
					break;
				case 8:
					value = va_arg(args, uint64_t);
					break;
				default:
					return; /* should not happen */
			}

			/* sign extension, if needed */
			if (is_signed) {
				int shift = 64 - 8 * bytelen;
				value = (uint64_t)(((int64_t)(value << shift)) >> shift);
			}

			if (alternate && value != 0 && (tolower(c) == 'x' || c == 'o' || tolower(c) == 'b')) {
				if (tolower(c) == 'x' || tolower(c) == 'b') {
					buffer[0] = '0';
					buffer[1] = c;
					format_integer(buffer + 2, sizeof(buffer) - 2, value, c);
				} else {
					buffer[0] = '0';
					format_integer(buffer + 1, sizeof(buffer) - 1, value, c);
				}
			} else
				/* format the number properly into our buffer */
				format_integer(buffer, sizeof(buffer), value, c);
		} else if (c == '%') {
			buffer[0] = '%';
			buffer[1] = '\0';
		} else
			__assert("conversion specifier unsupported", __FILE__, __LINE__);

		if (str == NULL)
			str = "(null)";

		/* if we are here, 'str' points to the content that must be
		 * outputted. handle padding and alignment now */

		slen = strlen(str);

		if (sign != '\0' || prec != -1)
			__assert("sign/precision unsupported", __FILE__, __LINE__);

		if (slen < width && !padLeft) {
			char padChar = padZero ? '0' : ' ';
			SendRepeat(buff, padChar, width - slen);
		}

		Send(buff, str, slen);

		if (slen < width && padLeft) {
			char padChar = padZero ? '0' : ' ';
			SendRepeat(buff, padChar, width - slen);
		}
	}
}

static int open_log_socket(void) {
	// ToDo: Ideally we want this to fail if the gid of the current
	// process is AID_LOGD, but will have to wait until we have
	// registered this in private/android_filesystem_config.h. We have
	// found that all logd crashes thus far have had no problem stuffing
	// the UNIX domain socket and moving on so not critical *today*.

	int log_fd = TEMP_FAILURE_RETRY(socket(PF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
	if (log_fd == -1)
		return -1;

	union {
		struct sockaddr addr;
		struct sockaddr_un addrUn;
	} u;
	memset(&u, 0, sizeof(u));
	u.addrUn.sun_family = AF_UNIX;
	strlcpy(u.addrUn.sun_path, _PATH_LOG, sizeof(u.addrUn.sun_path));

	if (TEMP_FAILURE_RETRY(connect(log_fd, &u.addr, sizeof(u.addrUn))) != 0) {
		close(log_fd);
		return -1;
	}

	return log_fd;
}

static int write_stderr(const char* tag, const char* msg) {
	struct iovec vec[4];
	vec[0].iov_base = (char*)tag;
	vec[0].iov_len = strlen(tag);
	vec[1].iov_base = (char*)": ";
	vec[1].iov_len = 2;
	vec[2].iov_base = (char*)msg;
	vec[2].iov_len = strlen(msg);
	vec[3].iov_base = (char*)"\n";
	vec[3].iov_len = 1;

	return TEMP_FAILURE_RETRY(writev(STDERR_FILENO, vec, 4));
}

int async_safe_write_log(int priority, const char* tag, const char* msg) {
	int main_log_fd = open_log_socket();
	if (main_log_fd == -1)
		// Try stderr instead.
		return write_stderr(tag, msg);

	struct iovec vec[6];
	char log_id = (priority == ANDROID_LOG_FATAL) ? LOG_ID_CRASH : LOG_ID_MAIN;
	vec[0].iov_base = &log_id;
	vec[0].iov_len = sizeof(log_id);
	uint16_t tid = INTERNAL_SYSCALL_CALL(gettid);
	vec[1].iov_base = &tid;
	vec[1].iov_len = sizeof(tid);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	struct log_time realtime_ts;
	realtime_ts.tv_sec = ts.tv_sec;
	realtime_ts.tv_nsec = ts.tv_nsec;
	vec[2].iov_base = &realtime_ts;
	vec[2].iov_len = sizeof(realtime_ts);

	vec[3].iov_base = &priority;
	vec[3].iov_len = 1;
	vec[4].iov_base = (char*)tag;
	vec[4].iov_len = strlen(tag) + 1;
	vec[5].iov_base = (char*)msg;
	vec[5].iov_len = strlen(msg) + 1;

	int result = TEMP_FAILURE_RETRY(writev(main_log_fd, vec, sizeof(vec) / sizeof(vec[0])));
	close(main_log_fd);
	return result;
}

int async_safe_format_log_va_list(int priority, const char* tag, const char* format, va_list args) {
	//ErrnoRestorer errno_restorer;
	char buffer[1024];
	struct BufferOutputStream os = BufferOutput(buffer, sizeof(buffer));
	out_vformat(&os, format, args);
	return async_safe_write_log(priority, tag, buffer);
}

int async_safe_format_log(int priority, const char* tag, const char* format, ...) {
	va_list args;
	va_start(args, format);
	int result = async_safe_format_log_va_list(priority, tag, format, args);
	va_end(args);
	return result;
}

// ======================
// syslog functions

void closelog(void) {
	syslog_log_tag = NULL;
	syslog_options = 0;
}

void openlog(const char* log_tag, int options, int /*facility*/) {
	syslog_log_tag = log_tag;
	syslog_options = options;
}

int setlogmask(int new_mask) {
	int old_mask = syslog_priority_mask;
	// 0 is used to query the current mask.
	if (new_mask != 0)
		syslog_priority_mask = new_mask;
	return old_mask;
}

void __vsyslog_internal(int priority, const char* fmt, va_list args, unsigned int mode_flags) {
	// Check whether we're supposed to be logging messages of this priority.
	if ((syslog_priority_mask & LOG_MASK(LOG_PRI(priority))) == 0)
		return;

	// What's our log tag?
	const char* log_tag = syslog_log_tag;
	if (log_tag == NULL)
		log_tag = __progname;

	// What's our Android log priority?
	priority &= LOG_PRIMASK;
	int android_log_priority;
	if (priority <= LOG_CRIT) // LOG_ALERT LOG_EMERG
		android_log_priority = ANDROID_LOG_FATAL;
	else if (priority == LOG_ERR)
		android_log_priority = ANDROID_LOG_ERROR;
	else if (priority <= LOG_NOTICE) // LOG_WARNING
		android_log_priority = ANDROID_LOG_WARN;
	else if (priority == LOG_INFO)
		android_log_priority = ANDROID_LOG_INFO;
	else // LOG_DEBUG
		android_log_priority = ANDROID_LOG_DEBUG;

	// We can't let async_safe_format_log do the formatting because it doesn't
	// support all the printf functionality.
	char log_line[1024];
	int n = __vsnprintf_internal(log_line, sizeof(log_line), fmt, args, mode_flags);
	if (n < 0) return;

	async_safe_format_log(android_log_priority, log_tag, "%s", log_line);
	if ((syslog_options & LOG_PERROR) != 0) {
		bool have_newline =
				(n > 0 && n < (int)sizeof(log_line) && log_line[n - 1] == '\n');
		dprintf(STDERR_FILENO, "%s: %s%s", log_tag, log_line, have_newline ? "" : "\n");
	}
}

void __syslog(int pri, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__vsyslog_internal(pri, fmt, ap, 0);
	va_end(ap);
}
ldbl_hidden_def(__syslog, syslog)
ldbl_strong_alias(__syslog, syslog)

void __vsyslog(int pri, const char *fmt, va_list ap) {
	__vsyslog_internal(pri, fmt, ap, 0);
}
ldbl_weak_alias(__vsyslog, vsyslog)

void ___syslog_chk(int pri, int flag, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	__vsyslog_internal(pri, fmt, ap, (flag > 0) ? PRINTF_FORTIFY : 0);
	va_end(ap);
}
ldbl_hidden_def(___syslog_chk, __syslog_chk)
ldbl_strong_alias(___syslog_chk, __syslog_chk)

void __vsyslog_chk(int pri, int flag, const char *fmt, va_list ap) {
	__vsyslog_internal(pri, fmt, ap, (flag > 0) ? PRINTF_FORTIFY : 0);
}
