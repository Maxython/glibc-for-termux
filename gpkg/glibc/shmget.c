#include <shmem-android.h>
#include <sys/msg.h>
#include <stddef.h>

/* Return an identifier for an shared memory segment of at least size SIZE
   which is associated with KEY.  */

int shmget(key_t key, size_t size, int flags) {
	(void) flags;

	ashv_check_pid();

	// Counter wrapping around at 15 bits.
	static size_t shmem_counter = 0;

	if (!ashv_listening_thread_id) {
		int sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (!sock) {
			DBG ("%s: cannot create UNIX socket: %s\n", __PRETTY_FUNCTION__, strerror(errno));
			errno = EINVAL;
			return -1;
		}
		int i;
		for (i = 0; i < 4096; i++) {
			struct sockaddr_un addr;
			int len;
			memset (&addr, 0, sizeof(addr));
			addr.sun_family = AF_UNIX;
			ashv_local_socket_id = (getpid() + i) & 0xffff;
			sprintf(&addr.sun_path[1], ANDROID_SHMEM_SOCKNAME, ashv_local_socket_id);
			len = sizeof(addr.sun_family) + strlen(&addr.sun_path[1]) + 1;
			if (bind(sock, (struct sockaddr *)&addr, len) != 0) continue;
			DBG("%s: bound UNIX socket %s in pid=%d\n", __PRETTY_FUNCTION__, addr.sun_path + 1, getpid());
			break;
		}
		if (i == 4096) {
			DBG("%s: cannot bind UNIX socket, bailing out\n", __PRETTY_FUNCTION__);
			ashv_local_socket_id = 0;
			errno = ENOMEM;
			return -1;
		}
		if (listen(sock, 4) != 0) {
			DBG("%s: listen failed\n", __PRETTY_FUNCTION__);
			errno = ENOMEM;
			return -1;
		}
		int* socket_arg = malloc(sizeof(int));
		*socket_arg = sock;
		pthread_create(&ashv_listening_thread_id, NULL, &ashv_thread_function, socket_arg);
	}

	int shmid = -1;

	pthread_mutex_lock(&mutex);
	char symlink_path[256];
	if (key != IPC_PRIVATE) {
		// (1) Check if symlink exists telling us where to connect.
		// (2) If so, try to connect and open.
		// (3) If connected and opened, done. If connection refused
		//     take ownership of the key and create the symlink.
		// (4) If no symlink, create it.
		sprintf(symlink_path, ASHV_KEY_SYMLINK_PATH, key);
		char path_buffer[256];
		char num_buffer[64];
		while (true) {
			int path_length = readlink(symlink_path, path_buffer, sizeof(path_buffer) - 1);
			if (path_length != -1) {
				path_buffer[path_length] = '\0';
				int shmid = atoi(path_buffer);
				if (shmid != 0) {
					int idx = ashv_find_local_index(shmid);

					if (idx == -1) {
						idx = ashv_read_remote_segment(shmid);
					}

					if (idx != -1) {
						pthread_mutex_unlock(&mutex);
						return shmem[idx].id;
					}
				}
				// TODO: Not sure we should try to remove previous owner if e.g.
				// there was a tempporary failture to get a soket. Need to
				// distinguish between why ashv_read_remote_segment failed.
				unlink(symlink_path);
			}
			// Take ownership.
			// TODO: HAndle error (out of resouces, no infinite loop)
			if (shmid == -1) {
				shmem_counter = (shmem_counter + 1) & 0x7fff;
				shmid = ashv_shmid_from_counter(shmem_counter);
				sprintf(num_buffer, "%d", shmid);
			}
			if (symlink(num_buffer, symlink_path) == 0) break;
		}
	}


	int idx = shmem_amount;
	char buf[256];
	sprintf(buf, ANDROID_SHMEM_SOCKNAME "-%d", ashv_local_socket_id, idx);

	shmem_amount++;
	if (shmid == -1) {
		shmem_counter = (shmem_counter + 1) & 0x7fff;
		shmid = ashv_shmid_from_counter(shmem_counter);
	}

	shmem = realloc(shmem, shmem_amount * sizeof(shmem_t));
	size = ROUND_UP(size, getpagesize());
	shmem[idx].size = size;
	shmem[idx].descriptor = ashmem_create_region(buf, size);
	shmem[idx].addr = NULL;
	shmem[idx].id = shmid;
	shmem[idx].markedForDeletion = false;
	shmem[idx].key = key;

	if (shmem[idx].descriptor < 0) {
		DBG("%s: ashmem_create_region() failed for size %zu: %s\n", __PRETTY_FUNCTION__, size, strerror(errno));
		shmem_amount --;
		shmem = realloc(shmem, shmem_amount * sizeof(shmem_t));
		pthread_mutex_unlock (&mutex);
		return -1;
	}
	//DBG("%s: ID %d shmid %x FD %d size %zu\n", __PRETTY_FUNCTION__, idx, shmid, shmem[idx].descriptor, shmem[idx].size);
	/*
	status = ashmem_set_prot_region (shmem[idx].descriptor, 0666);
	if (status < 0) {
		//DBG ("%s: ashmem_set_prot_region() failed for size %zu: %s %d\n", __PRETTY_FUNCTION__, size, strerror(status), status);
		shmem_amount --;
		shmem = realloc (shmem, shmem_amount * sizeof(shmem_t));
		pthread_mutex_unlock (&mutex);
		return -1;
	}
	*/
	/*
	status = ashmem_pin_region (shmem[idx].descriptor, 0, shmem[idx].size);
	if (status < 0) {
		//DBG ("%s: ashmem_pin_region() failed for size %zu: %s %d\n", __PRETTY_FUNCTION__, size, strerror(status), status);
		shmem_amount --;
		shmem = realloc (shmem, shmem_amount * sizeof(shmem_t));
		pthread_mutex_unlock (&mutex);
		return -1;
	}
	*/
	pthread_mutex_unlock(&mutex);

	return shmid;
}
