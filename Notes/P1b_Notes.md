# P1b

Sometimes people refer to the individual structure that stores information about a process as a **Process Control Block** (**PCB**), a fancy way of talking about a C structure that contains information about each process (also sometimes called a **process descriptor**).

We do not need to keep a count of the number of characters stored in the array since the first `NULL` character encountered indicates the end of the string. In our program, when the first `NULL` is reached we terminate the string output with a newline.

"C strings" will be null-terminated on any platform. That's how the standard C library functions determine the end of a string.

**'\0' not accepted as an empty "const char *"** 

```text
'\0' is a single character, not a string. Note the use of signal quote marks, THIS IS IMPORTANT. It is an 8bit value equivalent to the byte 0x00, or an integer 0x00000000. You could replace '\0' with 0x00 everywhere in your code and they would be exactly the same.
"" is a string containing one byte, '\0'. It's equivalent to a byte array like { 0x00 }. This array is allocate somewhere in memory, not a literal value in your program like the previous example.
```

**char * strdup(const char *s)**

strdup()会先用maolloc()配置与参数s 字符串相同的空间大小，然后将参数s 字符串的内容复制到该内存地址，然后把该地址返回。该地址最后可以利用free()来释放。

对应每一个process，在kernel有一个process structure存放相关信息，比如

```C
// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  volatile int pid;            // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};
```

#### File descriptor

Each process by default has 3 open file descriptors. Standard input, output and error.

In [Unix](https://en.wikipedia.org/wiki/Unix) and [related](https://en.wikipedia.org/wiki/Unix-like) computer operating systems, a **file descriptor** (**FD**, less frequently **fildes**) is an abstract indicator ([handle](https://en.wikipedia.org/wiki/Handle_(computing))) used to access a [file](https://en.wikipedia.org/wiki/File_(computing)) or other [input/output](https://en.wikipedia.org/wiki/Input/output) [resource](https://en.wikipedia.org/wiki/System_resource), such as a [pipe](https://en.wikipedia.org/wiki/Pipe_(Unix)) or [network socket](https://en.wikipedia.org/wiki/Network_socket). File descriptors form part of the [POSIX](https://en.wikipedia.org/wiki/POSIX) [application programming interface](https://en.wikipedia.org/wiki/Application_programming_interface). A file descriptor is a non-negative [integer](https://en.wikipedia.org/wiki/Integer), generally represented in the [C](https://en.wikipedia.org/wiki/C_(programming_language)) programming language as the type int (negative values being reserved to indicate "no value" or an error condition).

In simple words, when you open a file, the operating system creates an entry to represent that file and store the information about that opened file. So if there are 100 files opened in your OS then there will be 100 entries in OS (somewhere in kernel). These entries are represented by integers like (...100, 101, 102....). This entry number is the file descriptor. So it is just an integer number that uniquely represents an opened file in operating system. If your process opens 10 files then your Process table will have 10 entries for file descriptors.

A file descriptor is an opaque handle that is used in the interface between user and kernel space to identify file/socket resources. Therefore, when you use `open()` or `socket()` (system calls to interface to the kernel), you are given a file descriptor, which is an integer (it is actually an index into the processes u structure - but that is not important). Therefore, if you want to interface directly with the kernel, using system calls to `read()`, `write()`, `close()` etc. the handle you use is a file descriptor.

There is a layer of abstraction overlaid on the system calls, which is the `stdio` interface. This provides more functionality/features than the basic system calls do. For this interface, the opaque handle you get is a `FILE*`, which is returned by the `fopen()` call. There are many many functions that use the `stdio` interface `fprintf()`, `fscanf()`, `fclose()`, which are there to make your life easier. In C, `stdin`, `stdout`, and `stderr` are `FILE*`, which in UNIX respectively map to file descriptors `0`, `1` and `2`.



<code>pwd</code> 查看当前路径

```bash
vim = Vi IMproved
```

现在很多发行版直接把`vi`做成`vim`的软连接了，如果你直接执行`vi`欢迎界面上显示了`VIM - Vi IMproved`字样，就说明你实际上用的就是`vim`

**serial console** 系统控制台

System calls allow the operating system to run code on the behalf of user requests but in a protected manner, both by jumping into the kernel (in a very specific and restricted way) and also by simultaneously raising the privilege level of the hardware, so that the OS can perform certain restricted operations.

#### [qemu](https://pdos.csail.mit.edu/6.828/2010/labguide.html)

QEMU (quick emulator) 是一款可执行硬件虚拟化的（hardware virtualization）开源托管虚拟机（VMM）。

QEMU是一个托管的虚拟机镜像，它通过动态的二进制转换，模拟CPU，并且提供一组设备模型，使它能够运行多种未修改的客户机OS，可以通过与KVM (kernel-based virtual machine开源加速器) 一起使用进而接近本地速度运行虚拟机（接近真实计算机的速度）。

QEMU还可以为user-level的进程执行CPU仿真，进而允许了为一种架构编译的程序在另外一中架构上面运行（借由VMM的形式）。

<code>make qemu</code>

Build everything and start qemu with the VGA console in a new window and the serial console in your terminal. To exit, either close the VGA window or press `Ctrl-c` or `Ctrl-a x` in your terminal.

<code>make qemu-nox</code>

Like `make qemu`, but run with only the serial console. To exit, press `Ctrl-a x`. This is particularly useful over SSH connections to Athena dialups because the VGA window consumes a lot of bandwidth.



#### [xv6](https://www.cnblogs.com/YukiJohnson/archive/2012/10/27/2741836.html)

```bash
$ ls
FILES     Makefile~  bootother*  fs.img    initcode*  tools/  version
Makefile  README     fs/         include/  kernel/    user/   xv6.img
```

important directories:   user/    kernel/    include/    

programs don't exit in main, they call the System call <code>exit()</code>  in main which is a function call that eventually becomes a trap. 

>  **Note**: if not put <code>exit()</code> in the end of <code>main()</code> there will be some problem

In xv6 every program should end with  <code>exit()</code>.



#### how to add a new system call

1. create a new <code>.c</code> file under <code>user</code>.

   ```c
   #include "types.h"
   #include "user.h"
   
   int main(int argc, char *argv[]){
       printf(1,  "my name is %s\n", argv[0]);
       exit();
   } 
   ```

2. add the name of this call in <code>user/makefile.mk</code>:

   ```makefile
   # user programs
   USER_PROGS := \
           cat\
           echo\
           forktest\
           getopenedcount\              # add the new system call
           grep\
           init\
           kill\
           ln\
           ls\
           mkdir\
           rm\
           sh\
           stressfs\
           tester\
           usertests\
           wc\
           zombie
   
   USER_PROGS := $(addprefix user/, $(USER_PROGS))
   ```

3. input <code>make</code>:

   ```bash
   gcc  -I include -nostdinc -Wall -Werror -ggdb -fno-pic -fno-builtin -fno-strict-aliasing -fno-stack-protector -m32 -c -o user/getopenedcount.o user/getopenedcount.c
   ld  -m    elf_i386 -nostdlib --omagic --entry=main --section-start=.text=0x0 --output=user/bin/getopenedcount user/getopenedcount.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
   cp user/bin/getopenedcount fs/getopenedcount
   ```

   <code>ld</code> is the linker (linking editor). It accepts the compiled file and produce the object. It links some library objects as well, for example, <code>printf.o</code>.

4. <code>usys.S</code>: where all the system calls are.

   system call: assembly code, to do some setup for trap.

   ```assembly
   #include "syscall.h"
   #include "traps.h"
   
   #define SYSCALL(name) \      [a macro. send a name to this SYSCALL]
     .globl name; \
     name: \      [for this name, do the following three assembly instructions.]
       movl $SYS_ ## name, %eax; \  [%eax is one of the general purpose registers in x86]
       int $T_SYSCALL; \
       ret
   
   SYSCALL(fork)
   SYSCALL(exit)
   SYSCALL(wait)
   SYSCALL(pipe)
   SYSCALL(read)
   SYSCALL(write)
   SYSCALL(close)
   SYSCALL(kill)
   SYSCALL(exec)
   SYSCALL(open)
   SYSCALL(mknod)
   SYSCALL(unlink)
   SYSCALL(fstat)
   SYSCALL(link)
   SYSCALL(mkdir)
   SYSCALL(chdir)
   SYSCALL(dup)
   SYSCALL(getpid)
   SYSCALL(sbrk)
   SYSCALL(sleep)
   SYSCALL(uptime)
   ```

   If input <code>gcc -S user usys.S</code> will create assembly files instead of object files.

   <code>%eax</code> is one of the general purpose registers in x86, but also used to store return values in code. 

   ```movl $SYS_ ## name, %eax;```

   Each time, want to put some value (system call number) in  <code>%eax</code> and trap into the kernel. So that the kernel can realize its a system call, and which one it is, send to the correct address . Then enters kernel mode, do some task and return. Also, what returns will be put in <code>%eax</code>.

5. <code>user.h</code>

   ```bash
   $  grep -H exit user/*.h
   user/user.h:int exit(void) __attribute__((noreturn));
   ```

6. vector？？

   <code>jmp alltraps</code> jump to this routine

7. trapasm.S

   when trap: switch from user stack to kernel stack

   trap frame: all the stuff we need to save when we trap.



#### [gdb](http://sourceware.org/gdb/current/onlinedocs/gdb/)



#### [x86](http://www.cs.virginia.edu/~evans/cs216/guides/x86.html)

```assembly
.globl vector64
vector64:
  pushl $64
  jmp alltraps
```

`.global` makes the symbol visible to `ld`. If you define `symbol` in your partial program, its value is made available to other partial programs that are linked with it. Otherwise,`symbol` takes its attributes from a symbol of the same name from another file linked into the same program.
