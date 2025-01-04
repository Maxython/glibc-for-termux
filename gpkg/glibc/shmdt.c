#include <shmem-android.h>
#include <sys/mman.h>

/* Detach shared memory segment starting at address specified by SHMADDR
   from the caller's data segment.  */

int shmdt(const void* shmaddr) {
	ashv_check_pid();

	pthread_mutex_lock(&mutex);
	for (size_t i = 0; i < shmem_amount; i++) {
		if (shmem[i].addr == shmaddr) {
			if (munmap(shmem[i].addr, shmem[i].size) != 0) {
				DBG("%s: munmap %p failed\n", __PRETTY_FUNCTION__, shmaddr);
			}
			shmem[i].addr = NULL;
			DBG("%s: unmapped addr %p for FD %d ID %zu shmid %x\n", __PRETTY_FUNCTION__, shmaddr, shmem[i].descriptor, i, shmem[i].id);
			if (shmem[i].markedForDeletion || ashv_socket_id_from_shmid(shmem[i].id) != ashv_local_socket_id) {
				DBG ("%s: deleting shmid %x\n", __PRETTY_FUNCTION__, shmem[i].id);
				android_shmem_delete(i);
			}
			pthread_mutex_unlock(&mutex);
			return 0;
		}
	}
	pthread_mutex_unlock(&mutex);

	DBG("%s: invalid address %p\n", __PRETTY_FUNCTION__, shmaddr);
	/* Could be a remove segment, do not report an error for that. */
	return 0;
}
