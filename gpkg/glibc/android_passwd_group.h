#ifndef _ANDROID_PASSWD_GROUP_H
#define _ANDROID_PASSWD_GROUP_H

#include <pwd.h>
#include <grp.h>

struct android_id_info * find_android_id_info_by_id(unsigned id);
struct android_id_info * find_android_id_info_by_name(const char* name);
int is_oem_id_android(id_t id);
int is_valid_id_android(id_t id, int is_group);
id_t oem_id_from_name_android(const char* name);
id_t app_id_from_name_android(const char* name, int is_group);
void get_name_by_uid_android(uid_t uid, char *name_u);
void get_name_by_gid_android(gid_t gid, char *name_g);
struct passwd * get_passwd_android(const char* name, uid_t uid);
struct group * get_group_android(const char* name, gid_t gid);
struct passwd * getpwuid_android(uid_t uid);
struct group * getgrgid_android(gid_t gid);
struct passwd * getpwnam_android(const char* name);
struct group * getgrnam_android(const char* name);

#endif // _ANDROID_PASSWD_GROUP_H
