/* This script stores functions similar to those from
   the bionic library, thanks to which passwd/group
   structures are created.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "android_ids.h"
#include "android_passwd_group.h"

struct android_id_info * find_android_id_info_by_id(unsigned id) {
	for (size_t n = 0; n < android_id_count; ++n)
		if (android_ids[n].aid == id)
			return &android_ids[n];
	return NULL;
}

struct android_id_info * find_android_id_info_by_name(const char* name) {
	for (size_t n = 0; n < android_id_count; ++n)
		if (!strcmp(android_ids[n].name, name))
			return &android_ids[n];
	return NULL;
}

int is_oem_id_android(id_t id) {
	if (id >= AID_OEM_RESERVED_START && id < AID_EVERYBODY && find_android_id_info_by_id(id) == NULL)
		return 1;

	return (id >= AID_OEM_RESERVED_START && id <= AID_OEM_RESERVED_END) ||
		(id >= AID_OEM_RESERVED_2_START && id <= AID_OEM_RESERVED_2_END);
}

int is_valid_id_android(id_t id, int is_group) {
	if (id >= AID_USER_OFFSET)
		return 0;

	struct IdRange * ranges;
	size_t ranges_size;
	id_t appid = id % AID_USER_OFFSET;

	if (appid == AID_OVERFLOWUID)
		return 0;

	if (is_group) {
		ranges_size = sizeof(group_ranges)/sizeof(group_ranges[0]);
		ranges = group_ranges;
	} else {
		ranges_size = sizeof(user_ranges)/sizeof(user_ranges[0]);
		ranges = user_ranges;
	}

	if (appid < ranges[0].start)
		return 1;

	if (appid >= AID_SHARED_GID_START && appid <= AID_SHARED_GID_END && appid != id)
		return 0;

	for (size_t i = 0; i < ranges_size; ++i)
		if (appid >= ranges[i].start && appid <= ranges[i].end)
			return 1;

	return 0;
}

id_t oem_id_from_name_android(const char* name) {
	unsigned int id;
	if (sscanf(name, "oem_%u", &id) != 1) {
		return 0;
	}
	if (!is_oem_id_android(id)) {
		return 0;
	}
	return (id_t)id;
}

id_t app_id_from_name_android(const char* name, int is_group) {
	char* end;
	unsigned long userid;
	struct android_id_info* info;
	int is_shared_gid = 0;

	if (is_group && name[0] == 'a' && name[1] == 'l' && name[2] == 'l') {
		end = malloc(strlen(name));
		for (int i=3; i<strlen(name); i++)
			strncat(end, &name[i], 1);
		userid = 0;
		is_shared_gid = 1;
	} else if (name[0] == 'u' && isdigit(name[1])) {
		userid = strtoul(name+1, &end, 10);
	} else {
		return 0;
	}

	if (end[0] != '_' || end[1] == 0) {
		return 0;
	}

	unsigned long appid = 0;
	if (end[1] == 'a' && isdigit(end[2])) {
		if (is_shared_gid) {
			appid = strtoul(end+2, &end, 10) + AID_SHARED_GID_START;
			if (appid > AID_SHARED_GID_END) {
				return 0;
			}
		} else {
			appid = strtoul(end+2, &end, 10);
			if (is_group) {
				if (!strcmp(end, "_ext_cache")) {
					end += 10;
					appid += AID_EXT_CACHE_GID_START;
				} else if (!strcmp(end, "_ext")) {
					end += 4;
					appid += AID_EXT_GID_START;
				} else if (!strcmp(end, "_cache")) {
					end += 6;
					appid += AID_CACHE_GID_START;
				} else {
					appid += AID_APP_START;
				}
			} else {
				appid += AID_APP_START;
			}
		}
	} else if (end[1] == 'i' && isdigit(end[2])) {
		appid = strtoul(end+2, &end, 10) + AID_ISOLATED_START;
	} else if ((info = find_android_id_info_by_name(end + 1)) != NULL) {
		appid = info->aid;
		end += strlen(info->name) + 1;
	}

	if (end[0] != 0) {
		return 0;
	}

	if (userid > 1000) {
		return 0;
	}

	if (appid >= AID_USER_OFFSET) {
		return 0;
	}

	return (appid + userid*AID_USER_OFFSET);
}

void get_name_by_uid_android(uid_t uid, char *name_u) {
	uid_t appid = uid % AID_USER_OFFSET;
	uid_t userid = uid / AID_USER_OFFSET;
	struct android_id_info* info;

	if (appid >= AID_ISOLATED_START) {
		sprintf(name_u, "u%u_i%u", userid, appid - AID_ISOLATED_START);
	} else if (appid < AID_APP_START) {
		if ((info = find_android_id_info_by_id(appid)) != NULL)
			sprintf(name_u, "%s", info->name);
	} else {
		sprintf(name_u, "u%u_a%u", userid, appid - AID_APP_START);
	}
}

void get_name_by_gid_android(gid_t gid, char *name_g) {
	uid_t appid = gid % AID_USER_OFFSET;
	uid_t userid = gid / AID_USER_OFFSET;
	struct android_id_info* info;

	if (appid >= AID_ISOLATED_START) {
		sprintf(name_g, "u%u_i%u", userid, appid - AID_ISOLATED_START);
	} else if (userid == 0 && appid >= AID_SHARED_GID_START && appid <= AID_SHARED_GID_END) {
		sprintf(name_g, "all_a%u", appid - AID_SHARED_GID_START);
	} else if (appid >= AID_EXT_CACHE_GID_START && appid <= AID_EXT_CACHE_GID_END) {
		sprintf(name_g, "u%u_a%u_ext_cache", userid, appid - AID_EXT_CACHE_GID_START);
	} else if (appid >= AID_EXT_GID_START && appid <= AID_EXT_GID_END) {
		sprintf(name_g, "u%u_a%u_ext", userid, appid - AID_EXT_GID_START);
	} else if (appid >= AID_CACHE_GID_START && appid <= AID_CACHE_GID_END) {
		sprintf(name_g, "u%u_a%u_cache", userid, appid - AID_CACHE_GID_START);
	} else if (appid < AID_APP_START) {
		if ((info = find_android_id_info_by_id(appid)) != NULL)
			sprintf(name_g, "%s", info->name);
	} else {
		sprintf(name_g, "u%u_a%u", userid, appid - AID_APP_START);
	}
}

struct passwd * get_passwd_android(const char* name, uid_t uid) {
	static struct passwd res;

	res.pw_name = (char *)name;
	res.pw_passwd = (char *)"*";
	res.pw_uid = uid;
	res.pw_gid = uid;
	res.pw_gecos = (char *)"";
	res.pw_dir = (char *)APP_HOME_DIR;
	res.pw_shell = (char *)(APP_PREFIX_DIR "/bin/login");

	return &res;
}

struct group * get_group_android(const char* name, gid_t gid) {
	static struct group res;

	res.gr_name = (char *)name;
	res.gr_passwd = NULL;
	res.gr_gid = gid;
	res.gr_mem = *(char **[2]){(char **)name, NULL};

	return &res;
}

struct passwd * getpwuid_android(uid_t uid) {
	char* name_res = malloc(64);

	if (is_oem_id_android(uid))
		sprintf(name_res, "oem_%u", uid);
	else {
		if (!is_valid_id_android(uid, 0))
			return NULL;
		get_name_by_uid_android(uid, name_res);
		if (strlen(name_res) == 0)
			return NULL;
	}

	return get_passwd_android(name_res, uid);
}

struct group * getgrgid_android(gid_t gid) {
	char* name_res = malloc(64);

	if (is_oem_id_android(gid))
		sprintf(name_res, "oem_%u", gid);
	else {
		if (!is_valid_id_android(gid, 1))
			return NULL;
        	get_name_by_gid_android(gid, name_res);
		if (strlen(name_res) == 0)
			return NULL;
	}

	return get_group_android(name_res, gid);
}

struct passwd * getpwnam_android(const char* name) {
	uid_t uid;
	struct android_id_info* info;

	uid = app_id_from_name_android(name, 0);
	if (uid != 0)
		return get_passwd_android(name, uid);

	uid = oem_id_from_name_android(name);
	if (uid != 0)
		return get_passwd_android(name, uid);

	info = find_android_id_info_by_name(name);
	if (info != NULL)
		return get_passwd_android(name, info->aid);

	return NULL;
}

struct group * getgrnam_android(const char* name) {
	gid_t gid;
	struct android_id_info* info;

	gid = app_id_from_name_android(name, 1);
	if (gid != 0)
		return get_group_android(name, gid);

	gid = oem_id_from_name_android(name);
	if (gid != 0)
		return get_group_android(name, gid);

	info = find_android_id_info_by_name(name);
	if (info != NULL)
		return get_group_android(name, info->aid);

	return NULL;
}
