#ifndef __SHMEM_ANDROID
#define __SHMEM_ANDROID

#include <sys/socket.h>
#include <sys/un.h>
#include <paths.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <pthread.h>
#include <ipc_priv.h>
#include <stdlib.h>
#include <errno.h>

#ifdef ENABLE_DEBUG_SHMEM_ANDROID
# define DBG(...) printf(__VA_ARGS__)
#else
# define DBG(...)
#endif //ENABLE_DEBUG_SHMEM_ANDROID

#define __ASHMEMIOC 0x77
#define ASHMEM_NAME_LEN 256
#define ASHMEM_GET_SIZE _IO(__ASHMEMIOC, 4)
#define ASHMEM_SET_SIZE _IOW(__ASHMEMIOC, 3, size_t)
#define ASHMEM_SET_NAME _IOW(__ASHMEMIOC, 1, char[ASHMEM_NAME_LEN])

#define ASHV_KEY_SYMLINK_PATH _PATH_TMP "ashv_key_%d"
#define ANDROID_SHMEM_SOCKNAME "/dev/shm/%08x"
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

typedef struct {
	int id;
	void *addr;
	int descriptor;
	size_t size;
	bool markedForDeletion;
	key_t key;
} shmem_t;

extern pthread_mutex_t mutex;
extern shmem_t* shmem;
extern size_t shmem_amount;
extern int ashv_local_socket_id;
extern int ashv_pid_setup;
extern pthread_t ashv_listening_thread_id;

// PS: some functions are not available for including because they are used only inside shmem-android

//extern int ancil_send_fd(int sock, int fd) __THROW;
//libc_hidden_proto(ancil_send_fd)

//extern int ancil_recv_fd(int sock) __THROW;
//libc_hidden_proto(ancil_recv_fd)

//extern int ashmem_get_size_region(int fd) __THROW;
//libc_hidden_proto(ashmem_get_size_region)

extern int ashmem_create_region(char const* name, size_t size) __THROW;
libc_hidden_proto(ashmem_create_region)

extern void ashv_check_pid(void) __THROW;
libc_hidden_proto(ashv_check_pid)

extern int ashv_shmid_from_counter(unsigned int counter) __THROW;
libc_hidden_proto(ashv_shmid_from_counter)

extern int ashv_socket_id_from_shmid(int shmid) __THROW;
libc_hidden_proto(ashv_socket_id_from_shmid)

extern int ashv_find_local_index(int shmid) __THROW;
libc_hidden_proto(ashv_find_local_index)

extern void* ashv_thread_function(void* arg) __THROW;
libc_hidden_proto(ashv_thread_function)

extern void android_shmem_delete(int idx) __THROW;
libc_hidden_proto(android_shmem_delete)

extern int ashv_read_remote_segment(int shmid) __THROW;
libc_hidden_proto(ashv_read_remote_segment)

#endif /* __SHMEM_ANDROID */
