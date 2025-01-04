#include <shmem-android.h>
#include <sys/mman.h>

/* Attach the shared memory segment associated with SHMID to the data
   segment of the calling process.  SHMADDR and SHMFLG determine how
   and where the segment is attached.  */

void* shmat(int shmid, const void* shmaddr, int shmflg) {
	ashv_check_pid();

	int socket_id = ashv_socket_id_from_shmid(shmid);
	void *addr;

	pthread_mutex_lock(&mutex);

	int idx = ashv_find_local_index(shmid);
	if (idx == -1 && socket_id != ashv_local_socket_id) {
		idx = ashv_read_remote_segment(shmid);
	}

	if (idx == -1) {
		DBG ("%s: shmid %x does not exist\n", __PRETTY_FUNCTION__, shmid);
		pthread_mutex_unlock(&mutex);
		errno = EINVAL;
		return (void*) -1;
	}

	if (shmem[idx].addr == NULL) {
		shmem[idx].addr = mmap((void*) shmaddr, shmem[idx].size, PROT_READ | (shmflg == 0 ? PROT_WRITE : 0), MAP_SHARED, shmem[idx].descriptor, 0);
		if (shmem[idx].addr == MAP_FAILED) {
			DBG ("%s: mmap() failed for ID %x FD %d: %s\n", __PRETTY_FUNCTION__, idx, shmem[idx].descriptor, strerror(errno));
			shmem[idx].addr = NULL;
		}
	}
	addr = shmem[idx].addr;
	DBG ("%s: mapped addr %p for FD %d ID %d\n", __PRETTY_FUNCTION__, addr, shmem[idx].descriptor, idx);
	pthread_mutex_unlock (&mutex);

	return addr ? addr : (void *)-1;
}
