# How to create a static C library
### Background

This is a naive example of how to create a simple C library.

Static libraries are already compiled beforehand, and copied to executables of codes during their compilation. No re-compilation of libraries and no linking for static libraries are needed.

Static libraries are added to program before execution.

### Steps

First create `.h` to declare the functions and `.c` to implement the functions.

After implementation, compile the `.c` file to acquire an executable.

```sh
$ gcc -c naive.c
```

An object file is created: `naive.o`. Next using `ar` to create a static library:

```sh
# format: ar rcs [static library name] [list of object files ...]
$ ar rcs lib_naive.a naive.o
```

For the second term,

- `r` means add list of oject files to the  specified static library
- `c` means create the static library if not exist
- `s` means update indices of the static library

This command will create a static library `lib_naive.a`.

### Test static library

To test the library, add the corresponding `.h` in test file.

```C
#include "naive.h"
```

To compile, using the `-L` and `-l` flags:

```sh
$ gcc -L. test.c -lnaive -o test
```

- `-L` is used to specify the directory of this library. `.` means the current directory. The absolute path can also be used. Note that there is **NO** space between `-L` or `-l` and their specified values.
- `-l` is the linker option, with the object file name of the static library serve as suffix.

**Notice**: `-l` must be placed *after* the source code files, otherwise the linking will have no use.

Another way is to compile with the `-static` flag:

```sh
$ gcc test.c -static ./lib_naive.a -o test
```

Here the static library name is used.



### Reference

<https://my.oschina.net/daowuming/blog/775068>

