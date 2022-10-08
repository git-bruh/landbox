#define _GNU_SOURCE

#include "landbox.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum flag {
	FLAG_HELP = 0,
	FLAG_VERSION,
	FLAG_DEV,
	FLAG_PROC,
	FLAG_RO,
	FLAG_RO_TRY,
	FLAG_RW,
	FLAG_RW_TRY,
	FLAG_MAX
};

struct grant_args {
	const char *const path;
	enum landbox_perm perm_mask;
};

static const char *const version_str = "0.0.1";

/* clang-format off */
static const struct option longopts[FLAG_MAX + 1] = {
  {"help", no_argument, NULL, FLAG_HELP},
  {"version", no_argument, NULL, FLAG_VERSION},
  {"dev", no_argument, NULL, FLAG_DEV},
  {"proc", no_argument, NULL, FLAG_PROC},
  {"ro", required_argument, NULL, FLAG_RO},
  {"ro-try", required_argument, NULL, FLAG_RO_TRY},
  {"rw", required_argument, NULL, FLAG_RW},
  {"rw-try", required_argument, NULL, FLAG_RW_TRY},
  {0},
};
/* clang-format on */

#define R (LANDBOX_PERM_READ)
#define W (LANDBOX_PERM_WRITE)
#define X (LANDBOX_PERM_EXECUTE)

static const enum landbox_perm perm_masks[FLAG_MAX] = {
  [FLAG_DEV] = R | W | X,
  [FLAG_PROC] = R | W | X,
  [FLAG_RO] = R | X,
  [FLAG_RO_TRY] = R | X,
  [FLAG_RW] = R | W | X,
  [FLAG_RW_TRY] = R | W | X,
};

#undef R
#undef W
#undef X

static void
print_usage(const char *const argv0, FILE *stream) {
	fprintf(stream, "Usage: %s [option...] [--] command [args...]\n",
	  argv0 ? argv0 : "landbox");
	fprintf(stream,
	  "    --help            Print this help and exit\n"
	  "    --version         Print program version and exit\n"
	  "    --dev             Grant basic access to /dev\n"
	  "    --proc            Grant basic access to /proc\n"
	  "    --ro      PATH    Grant read-only access to PATH\n"
	  "    --ro-try  PATH    Same as --ro but ignore missing PATH\n"
	  "    --rw      PATH    Grant read-write access to PATH\n"
	  "    --rw-try  PATH    Same as --rw but ignore missing PATH\n");
}

static void
print_version(void) {
	printf("%s\n", version_str);
}

static int
grant_perm(const int handle, const char *const path,
  enum landbox_perm perm_mask, bool try) {
	int fd = open(path, O_PATH);

	if (fd == -1) {
		if (try && errno == ENOENT) {
			return 0;
		}

		return -1;
	}

	if (landbox_set_perm(handle, fd, perm_mask) == -1) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static struct grant_args
get_grant_args(const enum flag flag, const char *const path) {
	assert(flag >= 0 && flag < FLAG_MAX);

	return (struct grant_args) {.path = ((flag == FLAG_DEV)	   ? "/dev"
										 : (flag == FLAG_PROC) ? "/proc"
															   : path),
	  .perm_mask = perm_masks[flag]};
}

int
main(int argc, char *const argv[]) {
	if (argc < 2) {
		print_usage(argv[0], stderr);
		return EXIT_FAILURE;
	}

	int abi = landbox_init();

	if (abi == -1) {
		perror("Failed to initialize landbox");
		return EXIT_FAILURE;
	}

	int handle = landbox_open();

	if (handle == -1) {
		perror("Failed to open landbox handle");
		goto fail;
	}

	for (int flag;
		 (flag = getopt_long(argc, argv, "+", longopts, NULL)) != -1;) {
		bool try = false;

		switch (flag) {
		case FLAG_HELP:
			{
				print_usage(argv[0], stdout);
				goto success;
			}
		case FLAG_VERSION:
			{
				print_version();
				goto success;
			}
		case FLAG_DEV:
		case FLAG_PROC:
		case FLAG_RO:
		case FLAG_RW:
			break;
		case FLAG_RO_TRY:
		case FLAG_RW_TRY:
			try = true;
			break;
		default: /* '?' */
			printf("optarg: %d, %s\n", optind, optarg);
			print_usage(argv[0], stderr);
			goto fail;
		}

		const struct grant_args args = get_grant_args((enum flag) flag, optarg);

		assert(args.path);
		assert(args.perm_mask != 0);

		if (grant_perm(handle, args.path, args.perm_mask, try) == -1) {
			fprintf(stderr, "Failed to grant permissions for path %s: %s\n",
			  args.path, strerror(errno));
			goto fail;
		}
	}

	if (optind >= argc) {
		print_usage(argv[0], stderr);
		goto fail;
	}

	if (landbox_enforce(handle) == -1) {
		perror("Failed to enforce landbox rules");
		goto fail;
	}

	landbox_close(handle);
	handle = -1;

	if (execvp(argv[optind], &argv[optind]) == -1) {
		perror("execvp");
		goto fail;
	}

	/* Unreachable */
	abort();

success:
	landbox_close(handle);
	return EXIT_SUCCESS;

fail:
	landbox_close(handle);
	return EXIT_FAILURE;
}
