# landbox

Tiny helper library that wraps the Linux [landlock](https://landlock.io) API providing a few helpers over the raw syscall API.

# Building

```sh
make
make DESTDIR="$pkgdir" PREFIX=/usr install
```

Will build the library `liblandbox.a` and the example tool `landbox` and install them to `$DESTDIR/usr/{lib,include,usr}`.

# Usage

Include `landbox.h` for using the library, the public API consists of ~8 functions. Read the header file for more details. TLDR (error checking ommited):

```c
#define _GNU_SOURCE
#include <fcntl.h>
#include <landbox.h>
#include <unistd.h>

int main(void) {
  /* Makes the library check the available landlock ABI and filter out
   * unsupported flags accordingly */
  landbox_init();

  /* Opens a landlock handle to apply rules with. */
  int handle = landbox_open();

  /* Example */
  int fd = open("/usr", O_PATH);

  landbox_set_perm(handle, fd, LANDBOX_PERM_READ | LANDBOX_PERM_EXECUTE);

  close(fd);
  fd = open("/etc", O_PATH);

  landbox_set_perm(handle, fd, LANDBOX_PERM_READ);

  close(fd);
  fd = open("/tmp", O_PATH);

  /* Not granting execute permissions */
  landbox_set_perm(handle, fd, LANDBOX_PERM_READ | LANDBOX_PERM_WRITE);

  close(fd);

  /* Actually enforce the rules */
  landbox_enforce(handle);

  landbox_close(handle);

  execv("/bin/sh", (char *[]){"/bin/sh", NULL});
}
```

The predefined permission related enums `LANDBOX_PERM_{READ,WRITE,EXECUTE}` internally map to a bitmask of the corresponding landlock constants, to the extent supported by the ABI determined at runtime.

A few helper functions like `landbox_get_raw_perms` and `landbox_filter_raw_perms` are also provided along with syscall wrappers if the macro `LANDBOX_SYSCALL_WRAPPERS` is defined, which also expose the aforementioned information.

Running the above program:

```sh
λ cc example.c -Iinclude ./liblandbox.a
λ ./a.out
λ pwd
/home/testuser/Development/Repos/landbox
λ ls
ls: can't open '.': Permission denied
λ ls /mnt
ls: can't open '/mnt': Permission denied
λ ls /usr
bin      etc      include  lib      lib64    libexec  local    man      sbin     share
λ cat /etc/passwd
root:x:0:0:root:/root:/bin/sh
nobody:x:99:99:Unprivileged User:/dev/null:/bin/false
testuser:x:1000:1000:Linux User,,,:/home/testuser:/bin/sh
λ cd /tmp
λ printf '#!/bin/sh\necho test\n' > exec.sh
λ chmod +x exec.sh
λ ./exec.sh
/bin/sh: ./exec.sh: Permission denied
λ # LANDBOX_PERM_EXECUTE was not granted for /tmp
```

A sample program `landbox` is also provided:

```sh
λ ./landbox --help
Usage: ./landbox [option...] [--] command [args...]
    --help            Print this help and exit
    --version         Print program version and exit
    --dev             Grant basic access to /dev
    --proc            Grant basic access to /proc
    --ro      PATH    Grant read-only access to PATH
    --ro-try  PATH    Same as --ro but ignore missing PATH
    --rw      PATH    Grant read-write access to PATH
    --rw-try  PATH    Same as --rw but ignore missing PATH
```
