# File System Checking

The file system checker works when the operating system is not working. Therefore, the checking process does not rely on the xv6 system.

### Notes about  FS checker

#### 1. fs.img creation

The implementation is inside a c program `mkfs.c`. 

```sh
./tools/mkfs fs.img fs
```

this just copy what's inside `fs` into the `fs.img`.

```sh
$ ls fs
README  forktest*  kill*  mkdir*  stressfs*   wc*
cat*    grep*      ln*    rm*     tester*     zombie*
echo*   init*      ls*    sh*     usertests*
```

Used by the virtual machine when boot up:

```sh
/p/course/cs537-remzi/ta/tools/qemu -nographic -hdb fs.img xv6.img -smp 2
```



#### 2. understanding file image

One way to view a file image in a readable way is to use `hexdump`. 

```sh
$ hexdump -v -C ../fs.img | less
```

The structure of a file can be read and stored in a structure `stat`.  For the xv6 file system image, it is basically set up as:

```text
unused | superblock | inode blocks | bitmap | data blocks
```

One reason that the first block is unused might be that unused bytes are set to 0 in the file system, so the first block is left unused to avoid any mistake.



#### 3. [mmap](<http://man7.org/linux/man-pages/man2/mmap.2.html>)

`mmap()` is a way to map the content of a file into the virtual memory for read or modification.

If only the read operation is needed, the file could be closed as soon as `mmap()` returns.

```C
mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
```

To be able to modify the original file, the file descriptor for the original file should have permission to write and for `mmap()` the permission of write should be set as well as the flag for map sharing.

```c
mmap(NULL, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); 
```



#### 4. Compile with external header directories

To include header files from other directories, the `-iquote` flag is set.

```sh
gcc -iquote ../include -Wall -Werror -ggdb -o xcheck xcheck.c
```

See [manual](<https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html>) for more details.



#### 5. check specific bit in a byte

In memory data are **byte-addressable**, therefore to check a specific bit should use bit operation. To check the $N^{th}$ bit in `NUM`, use the following mask:

```C
NUM & (1<<N)
```

