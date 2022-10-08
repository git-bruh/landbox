#ifndef LANDBOX_H
#define LANDBOX_H

#ifdef LANDBOX_SYSCALL_WRAPPERS
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <linux/landlock.h>
#include <stddef.h>
#include <syscall.h>
#include <unistd.h>

#ifndef LANDLOCK_ACCESS_FS_REFER
#define LANDLOCK_ACCESS_FS_REFER (1ULL << 13)
#endif

#ifndef LANDLOCK_ACCESS_FS_TRUNCATE
#define LANDLOCK_ACCESS_FS_TRUNCATE (1ULL << 14)
#endif

#ifndef landlock_create_ruleset
static inline int
landlock_create_ruleset(const struct landlock_ruleset_attr *const attr,
  const size_t size, const __u32 flags) {
	return (int) syscall(__NR_landlock_create_ruleset, attr, size, flags);
}
#endif

#ifndef landlock_add_rule
static inline int
landlock_add_rule(const int ruleset_fd, const enum landlock_rule_type rule_type,
  const void *const rule_attr, const __u32 flags) {
	return (int) syscall(
	  __NR_landlock_add_rule, ruleset_fd, rule_type, rule_attr, flags);
}
#endif

#ifndef landlock_restrict_self
static inline int
landlock_restrict_self(const int ruleset_fd, const __u32 flags) {
	return (int) syscall(__NR_landlock_restrict_self, ruleset_fd, flags);
}
#endif
#endif

#include <stdbool.h>

enum landbox_perm {
	LANDBOX_PERM_READ = 1 << 1,
	LANDBOX_PERM_WRITE = 1 << 2,
	LANDBOX_PERM_EXECUTE = 1 << 3,
};

/* Must be the first function to be called, initializes
 * global state (unsupported flags)
 * Returns ABI version, or -1 on failure */
int
landbox_init(void);

/* Open a landlock handle, returns -1 on failure */
int
landbox_open(void);

/* Apply landlock permissions to a fd
 * perm_mask is a mask of enum landbox_perm
 * Returns -1 on failure */
int
landbox_set_perm(int handle, int fd, enum landbox_perm perm_mask);

/* Enforce the landlock ruleset, called after setting perms for all fds
 * Returns -1 on failure */
int
landbox_enforce(int handle);

void
landbox_close(int handle);

/* -1 on error, 0 for false and 1 for true */
int
landbox_util_fd_is_dir(int fd);

/* Helpers for syscall wrappers */
#ifdef LANDBOX_SYSCALL_WRAPPERS
/* Get raw permissions to be passed to landbox_filter_raw_perms */
int
landbox_get_raw_perms(enum landbox_perm perm_mask);

/* Make the raw permissions suitable for passing to landlock_add_rule
 * Filters out flags unsupported by the current landlock ABI and also
 * preserves only file-related flags if `is_dir` is false
 * is_dir indicates whether the file corresponding to the requested permissions
 * is a directory, must be set to true if flags are to be passed to
 * landlock_create_ruleset */
int
landbox_filter_raw_perms(int perms, bool is_dir);
#endif
#endif
