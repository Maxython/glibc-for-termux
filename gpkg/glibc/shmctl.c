#include <shmem-android.h>
#include <stdarg.h>
#include <shlib-compat.h>
#include <asm-generic/ipcbuf.h>
#include <asm-generic/shmbuf.h>

int __shmctl64(int shmid, int cmd, struct __shmid64_ds *buf) {
	ashv_check_pid();

	if (cmd == IPC_RMID) {
		DBG("%s: IPC_RMID for shmid=%x\n", __PRETTY_FUNCTION__, shmid);
		pthread_mutex_lock(&mutex);
		int idx = ashv_find_local_index(shmid);
		if (idx == -1) {
			DBG("%s: shmid=%x does not exist locally\n", __PRETTY_FUNCTION__, shmid);
			/* We do not rm non-local regions, but do not report an error for that. */
			pthread_mutex_unlock(&mutex);
			return 0;
		}

		if (shmem[idx].addr) {
			// shmctl(2): The segment will actually be destroyed only
			// after the last process detaches it (i.e., when the shm_nattch
			// member of the associated structure shmid_ds is zero.
			shmem[idx].markedForDeletion = true;
		} else {
			android_shmem_delete(idx);
		}
		pthread_mutex_unlock(&mutex);
		return 0;
	} else if (cmd == IPC_STAT) {
		if (!buf) {
			DBG ("%s: ERROR: buf == NULL for shmid %x\n", __PRETTY_FUNCTION__, shmid);
			errno = EINVAL;
			return -1;
		}

		pthread_mutex_lock(&mutex);
		int idx = ashv_find_local_index(shmid);
		if (idx == -1) {
			DBG ("%s: ERROR: shmid %x does not exist\n", __PRETTY_FUNCTION__, shmid);
			pthread_mutex_unlock (&mutex);
			errno = EINVAL;
			return -1;
		}
		/* Report max permissive mode */
		memset(buf, 0, sizeof(struct shmid_ds));
		buf->shm_segsz = shmem[idx].size;
		buf->shm_nattch = 1;
		buf->shm_perm.__key = shmem[idx].key;
		buf->shm_perm.uid = geteuid();
		buf->shm_perm.gid = getegid();
		buf->shm_perm.cuid = geteuid();
		buf->shm_perm.cgid = getegid();
		buf->shm_perm.mode = 0666;
		buf->shm_perm.__seq = 1;

		pthread_mutex_unlock (&mutex);
		return 0;
	}

	DBG("%s: cmd %d not implemented yet!\n", __PRETTY_FUNCTION__, cmd);
	errno = EINVAL;
	return -1;
}

#if __TIMESIZE != 64
libc_hidden_def(__shmctl64)

static void shmid_to_shmid64(struct __shmid64_ds *shm64, const struct shmid_ds *shm) {
	shm64->shm_perm   = shm->shm_perm;
	shm64->shm_segsz  = shm->shm_segsz;
	shm64->shm_atime  = shm->shm_atime | ((__time64_t) shm->__shm_atime_high << 32);
	shm64->shm_dtime  = shm->shm_dtime | ((__time64_t) shm->__shm_dtime_high << 32);
	shm64->shm_ctime  = shm->shm_ctime | ((__time64_t) shm->__shm_ctime_high << 32);
	shm64->shm_cpid   = shm->shm_cpid;
	shm64->shm_lpid   = shm->shm_lpid;
	shm64->shm_nattch = shm->shm_nattch;
}

static void shmid64_to_shmid(struct shmid_ds *shm, const struct __shmid64_ds *shm64) {
	shm->shm_perm         = shm64->shm_perm;
	shm->shm_segsz        = shm64->shm_segsz;
	shm->shm_atime        = shm64->shm_atime;
	shm->__shm_atime_high = 0;
	shm->shm_dtime        = shm64->shm_dtime;
	shm->__shm_dtime_high = 0;
	shm->shm_ctime        = shm64->shm_ctime;
	shm->__shm_ctime_high = 0;
	shm->shm_cpid         = shm64->shm_cpid;
	shm->shm_lpid         = shm64->shm_lpid;
	shm->shm_nattch       = shm64->shm_nattch;
}

int __shmctl(int shmid, int cmd, struct shmid_ds *buf) {
	struct __shmid64_ds shmid64, *buf64 = NULL;
	if (buf != NULL) {
		if (cmd == IPC_INFO || cmd == SHM_INFO)
			buf64 = (struct __shmid64_ds *) buf;
		else {
			shmid_to_shmid64 (&shmid64, buf);
			buf64 = &shmid64;
		}
	}

	int ret = __shmctl64 (shmid, cmd, buf64);
	if (ret < 0)
		return ret;

	switch (cmd) {
		case IPC_STAT:
		case SHM_STAT:
		case SHM_STAT_ANY:
			shmid64_to_shmid (buf, buf64);
	}

	return ret;
}
#endif

#ifndef DEFAULT_VERSION
# ifndef __ASSUME_SYSVIPC_BROKEN_MODE_T
#  define DEFAULT_VERSION GLIBC_2_2
# else
#  define DEFAULT_VERSION GLIBC_2_31
# endif
#endif

versioned_symbol(libc, __shmctl, shmctl, DEFAULT_VERSION);
