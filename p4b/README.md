# P4b specification

## Thread library

The thread library should be available as part of every program that runs in xv6. Thus, their declaration should be added to `user/user.h` and the actual code to implement the library routines should be in `user/ulib.c`.

### core functions

####  `clone()`

`clone()` is similar to `fork()` in many ways, as they are both allocating a new "process" from the `ptable`. The main difference is that `clone ` creates a thread which shares the same address space as the parent. The default address space layout of xv6 is as below:

```text
USERTOP = 0xA0000	 	----------------------------
 															(free)
			
										----------------------------
 															Heap 
													(grows upwards)
										----------------------------
 															Stack
 													(grows downwards)
											(fixed-sized, one page)	
 										----------------------------
															Code
ADDR = 0x0					----------------------------
```

To create a thread for `clone()`, `thread_create()` passes the address of stack to `clone()`. However, since the stack for this new thread is allocated by `malloc()`, it gives the loweset address. To make it grow downwards, in `clone()`, the stack pointer should be pointed to the top of this page.

In `thread_create`, one small thing to notice is that stack pointer should be aligned before sending to `clone()`. A simple bit arithmetic can achieve this:

```C
#define MYPAGE 0x1000
stack = (void *)((uint)(stack + MYPAGE -1) & ~(MYPAGE -1));
```

Cloned process share the same page directories with its parent thread. Therefore, when the size of the program grows (in whichever thread or main thread), the concurrent issue needs to be dealt with. The space of a process is increased (decreased) by calling `growproc()`, therefore, to ensure concurrency, each time the size of process is accessed lock should be held.



#### `join()`

`join` is similar to `wait` except that it only waits for threads, and it never needs to free the page directories. However, in `exit())`, if the parent process exit before its child threads, the children will be adopted by `initproc`, and the page table still should not be freed. Therefore, small changes should be made to inform threads whether parent process exits, and whether the page directories should be freed.





## Lock

A simple [ticket lock](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf) is also implemented in this project. There is a type `lock_t` that one uses to declare a lock,  `void lock_init(lock_t *)` to initialize it, and two routines `void lock_acquire(lock_t *)` and `void lock_release(lock_t *)`to acquire and release the lock. The ticket lock creates an atomic fetch-and-add routine using the x86 `xaddl` instruction.

### References

[wiki Fetch-and-add](<https://en.wikipedia.org/wiki/Fetch-and-add>)

[assembler instructions in C](<https://en.wikipedia.org/wiki/Fetch-and-add>)

### Assembly

With extended `asm` you can read and write C variables from assembler and perform jumps from assembler code to C labels. Extended `asm` syntax uses colons (‘:’) to delimit the operand parameters after the assembler template:

```C
asm asm-qualifiers ( AssemblerTemplate 
                 : OutputOperands 
                 [ : InputOperands
                 [ : Clobbers ] ])

asm asm-qualifiers ( AssemblerTemplate 
                      : 
                      : InputOperands
                      : Clobbers
                      : GotoLabels)
```

#### Qualifiers

#####`volatile`

The typical use of extended `asm` statements is to manipulate input values to produce output values. However, your `asm` statements may also produce side effects. If so, you may need to use the `volatile` qualifier to disable certain optimizations.

##### `inline`

If you use the `inline` qualifier, then for inlining purposes the size of the `asm` statement is taken as the smallest size possible

##### `goto`

This qualifier informs the compiler that the `asm` statement may perform a jump to one of the labels listed in the GotoLabels.

#### Parameters

##### AssemblerTemplate

This is a literal string that is the template for the assembler code. It is a combination of fixed text and tokens that refer to the input, output, and goto parameters. 

##### OutputOperands

A comma-separated list of the C variables **modified** by the instructions in the AssemblerTemplate. An empty list is permitted.

##### InputOperands

A comma-separated list of C expressions read by the instructions in the AssemblerTemplate. An empty list is permitted.

#### constraint

A string constant specifying constraints on the placement of the operand. Output constraints must begin with either ‘=’ (a variable overwriting an existing value) or ‘+’ (when reading and writing).



