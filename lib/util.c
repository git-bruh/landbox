#include <sys/stat.h>

/* -1 means to check errno. */
int
landbox_util_fd_is_dir(const int fd) {
	struct stat statbuf;

	if (fstat(fd, &statbuf) == -1) {
		return -1;
	}

	return S_ISDIR(statbuf.st_mode);
}
