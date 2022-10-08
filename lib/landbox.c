#define LANDBOX_SYSCALL_WRAPPERS
#include "landbox.h"

#include <sys/prctl.h>

enum {
	PERM_FILE = LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_WRITE_FILE
			  | LANDLOCK_ACCESS_FS_READ_FILE,
	PERM_READ = LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR,
	PERM_WRITE = LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_REMOVE_DIR
			   | LANDLOCK_ACCESS_FS_REMOVE_FILE | LANDLOCK_ACCESS_FS_MAKE_CHAR
			   | LANDLOCK_ACCESS_FS_MAKE_DIR | LANDLOCK_ACCESS_FS_MAKE_REG
			   | LANDLOCK_ACCESS_FS_MAKE_SOCK | LANDLOCK_ACCESS_FS_MAKE_FIFO
			   | LANDLOCK_ACCESS_FS_MAKE_BLOCK | LANDLOCK_ACCESS_FS_MAKE_SYM
			   | LANDLOCK_ACCESS_FS_REFER | LANDLOCK_ACCESS_FS_TRUNCATE,
	PERM_EXECUTE = LANDLOCK_ACCESS_FS_EXECUTE
};

static int g_unsupported_perms = 0;

int
landbox_get_raw_perms(const enum landbox_perm perm_mask) {
	int perms = 0;

	if (perm_mask & LANDBOX_PERM_READ) {
		perms |= PERM_READ;
	}

	if (perm_mask & LANDBOX_PERM_WRITE) {
		perms |= PERM_WRITE;
	}

	if (perm_mask & LANDBOX_PERM_EXECUTE) {
		perms |= PERM_EXECUTE;
	}

	return perms;
};

int
landbox_filter_raw_perms(int perms, const bool is_dir) {
	if (!is_dir) {
		perms &= PERM_FILE;
	}

	perms &= ~g_unsupported_perms;

	return perms;
}

int
landbox_init(void) {
	int abi = landlock_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);

	if (abi < 0) {
		return -1;
	}

	if (abi < 3) {
		g_unsupported_perms |= LANDLOCK_ACCESS_FS_TRUNCATE;
	}

	if (abi < 2) {
		g_unsupported_perms |= LANDLOCK_ACCESS_FS_REFER;
	}

	return abi;
}

int
landbox_open(void) {
	struct landlock_ruleset_attr ruleset_attr = {
	  .handled_access_fs = landbox_filter_raw_perms(
		landbox_get_raw_perms(PERM_READ | PERM_WRITE | PERM_EXECUTE), true),
	};

	return landlock_create_ruleset(&ruleset_attr, sizeof(ruleset_attr), 0);
}

int
landbox_set_perm(
  const int handle, const int fd, const enum landbox_perm perm_mask) {
	int is_dir = landbox_util_fd_is_dir(fd);

	if (is_dir == -1) {
		return -1;
	}

	struct landlock_path_beneath_attr path_beneath = {
	  .parent_fd = fd,
	  .allowed_access
	  = landbox_filter_raw_perms(landbox_get_raw_perms(perm_mask), is_dir),
	};

	return landlock_add_rule(
	  handle, LANDLOCK_RULE_PATH_BENEATH, &path_beneath, 0);
}

int
landbox_enforce(const int handle) {
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1
		|| landlock_restrict_self(handle, 0) == -1) {
		return -1;
	}

	return 0;
}

void
landbox_close(int handle) {
	close(handle);
}
