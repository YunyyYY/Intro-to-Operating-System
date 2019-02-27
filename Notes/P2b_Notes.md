# Scheduler

## Preliminary

### xv6 with gdb mode

Using two terminals in the same directory. Use continue to continue running the kernel. To get control back to gdb, use `ctrl+c`.

```sh
(gdb) continue
Continuing.
^C
Thread 1 received signal SIGINT, Interrupt.
The target architecture is assumed to be i386
=> 0x104774 <acquire+58>:       add    $0x10,%esp
0x00104774 in acquire (lk=0x10f3a0 <ptable>) at kernel/spinlock.c:33
33        while(xchg(&lk->locked, 1) != 0)
(gdb) break trap.c: 35
Breakpoint 1 at 0x105e97: file kernel/trap.c, line 35.
(gdb) c
Continuing.
[Switching to Thread 2]
=> 0x105e97 <trap+9>:   mov    0x8(%ebp),%eax

Thread 2 hit Breakpoint 1, trap (tf=0xff1f54) at kernel/trap.c:37
37        if(tf->trapno == T_SYSCALL){
(gdb) n
[Switching to Thread 1]
=> 0x105e97 <trap+9>:   mov    0x8(%ebp),%eax

Thread 1 hit Breakpoint 1, trap (tf=0xffff84) at kernel/trap.c:37
37        if(tf->trapno == T_SYSCALL){
(gdb) c
Continuing.

Thread 2 received signal SIGTRAP, Trace/breakpoint trap.
[Switching to Thread 2]
=> 0x102ed2 <lapiceoi>: push   %ebp
lapiceoi () at kernel/lapic.c:120
120     {

```

Timer interrupt: implemented in hardware. H/W raises the  interrupt, os handles it. 

We can add an output line `cprintf("process chosen to run %s, pid %d\n", p->name, p->pid);` in `scheduler()`to see its  scheduling:

```c
void scheduler(void)
{
  struct proc *p;

  for(;;){
    sti();  // Enable interrupts on this processor.

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      cprintf("process chosen to run %s, pid %d\n", p->name, p->pid);         // add  ***
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      proc = 0;
    }
    release(&ptable.lock);
  }
}
```

A couple of things will be printed:

```sh
cpu1: starting
cpu0: starting
process chosen to run initcode, pid 1
...
process chosen to run initcode, pid 1
process chosen to run init, pid 1
...
process chosen to run init, pid 1                        (1)
init: starting sh
process chosen to run init, pid 2                        (2)
...
process chosen to run init, pid 2
process chosen to run sh, pid 2
...
process chosen to run sh, pid 2
$
```

When boots up, first run the process `initcode`, Even before a shell gets going. The only way to create a process in unix is `fork()`. The only way to call a shell is to call `fork()` within `init`. Once `init` starting `sh`, pid of `init` becomes 2. Because at this time, `init` forks a new process, and at first its name is the same as its parent. When `sh` begins to execute, its name changes to "sh". Now, shell is waiting for input, but nothing get printed. Since `sheduler()` only deals with processes that is `RUNNABLE`, and apparently shell is blocked with keyboard input. When a new command is entered, the 



### Procedure

`kernel/trap.c`, `trap()`

`kernel/proc.c`, `yield()`. Give up the cpu for one scheduling round. call `sched()`.  







###C programming

####Key word `volatile`

A variable should be declared volatile whenever its value could change unexpectedly. In practice, only three types of variables could change:

1. Memory-mapped peripheral registers
2. Global variables modified by an interrupt service routine
3. Global variables accessed by multiple tasks within a multi-threaded application



### xv6 Organization

**monolithic kernel**: used by many Unix operating systems. A **Monolithic kernel** is an OS architecture where the entire operating system (which includes the device drivers, file system, and the application IPC) is working in **kernel** space. **Monolithic kernels** are able to dynamically load (and unload) executable modules at runtime.

Unix transparently switches hardware processors among processes, saving and restoring register state as necessary, so that applications don’t have to be aware of time sharing. 

Unix processes use `exec` to build up their memory image, instead of directly interacting with physical memory.

To achieve strong isolation, the operating system must arrange that applications cannot modify (or even
read) the operating system’s data structures and instructions and that applications cannot access other process’s memory.

The unit of isolation in xv6 (as in other Unix operating systems) is a *process*. A process provides a program with what appears to be a private memory system, or *address space*, which other processes cannot
read or write. A process also provides the program with what appears to be its own CPU to execute the program’s instructions.

### xv6 Scheduling

First, xv6’s sleep and wakeup mechanism switches when a process waits for device or pipe I/O to complete, or waits for a child to exit, or waits in the sleep system call. Second, xv6 periodically forces a switch when a process is executing user instructions. **This multiplexing creates the illusion that each process has its own CPU, just as xv6 uses the memory allocator and hardware page tables to create the illusion that each process has its own memory.**

#### x86 backgrounds

```text
______Volatile______
Some registers are typically volatile across functions, and others remain unchanged. When a function is called, there is no guarantee that volatile registers will retain their value when the function returns, and it's the function's responsibility to preserve non-volatile registers.

########### selected 32 bits registers ############

_____General_Purpose_Registers_____
** eax **
eax is a 32-bit general-purpose register with two common uses: to store the return value of a function and as a special register for certain calculations. 

** ebp **
ebp is a non-volatile general-purpose register that has two distinct uses depending on compile settings: it is either the frame pointer (if compilation is not optimized) or a general purpose register (if compilation is optimized).

** esp **   (think of as stack pointer)
esp is a special register that stores a pointer to the top of the stack (the top is actually at a lower virtual address than the bottom as the stack grows downwards in memory towards the heap). Math is rarely done directly on esp, and the value of esp must be the same at the beginning and the end of each function.

_____Special_Purpose_Registers_____
** eip **   (think of as instruction pointer)
eip, or the instruction pointer, is a special-purpose register which stores a pointer to the address of the instruction that is currently executing. Making a jump is like adding to or subtracting from the instruction pointer.
```

**Questions**

1. how to switch from one process to another?
2. how to switch transparently to user processes?
   Xv6 uses the standard technique of driving context switches with timer interrupts.
3. many CPUs may be switching among processes concurrently, and a locking plan is necessary to avoid races
4. a process’s memory and other resources must be freed when the process exits, but it cannot do all of this itself because (for example) it can’t free its own kernel stack while still using it.

#### Context switch

Switching from one thread to another involves saving the old thread’s CPU registers, and restoring the previously-saved registers of the new thread; the fact that %esp and %eip are saved and restored means that the CPU will switch stacks and switch what code it is executing.

When it is time for a process to give up the CPU, the process’s kernel thread calls swtch to save its own context and return to the scheduler context. Each context is represented by a `struct context*`, a pointer to a structure stored on the kernel stack involved.

In our example, sched called swtch to switch to cpu->scheduler, the per-CPU scheduler context. That context had been saved by scheduler’s call to swtch. When the swtch we have been tracing returns, it returns not to sched but to scheduler, and its stack pointer points at the current CPU’s scheduler stack.

#### Sceduling

A process that wants to give up the CPU must acquire the process table lock `ptable.lock`, release any other locks it is holding, update its own state (`proc->state`), and then call `sched`. Sched double-checks those conditions and then an implication of those conditions: **since a lock is held, the CPU should be running with interrupts disabled**. 

Finally, sched calls `swtch` to save the current context in `proc->context`and switch to the scheduler context in `cpu->scheduler`. **`swtch` returns on the scheduler’s stack as though scheduler’s `swtch` had returned. The scheduler continues the for loop, finds a process to run, switches to it, and the cycle repeats.**

A kernel thread always gives up its processor in sched and always switches to the same location in the scheduler, which (almost) always switches to some kernel thread that previously called sched.

Scheduler runs a simple loop: find a process to run, run it until it yields, repeat. The scheduler loops over the process table looking for a runnable process, one that has `p->state == RUNNABLE`. Once it finds a process, it sets the per-CPU current process variable proc, switches to the process’s page table with switchuvm, marks the process as `RUNNING`, and then calls swtch to start running it.

To switch transparently between processes, the kernel suspends the currently running thread and resumes
another process’s thread. Much of the state of a thread (local variables, function call return addresses) is stored on the thread’s stacks. Each process has two stacks: a user stack and a kernel stack (p->kstack). 

- When the process is executing user instructions, only its user stack is in use, and its kernel stack is empty. 
- When the process enters the kernel (for a system call or interrupt), the kernel code executes on the process’s kernel stack; while a process is in the kernel, its **user stack still contains saved data**, but isn’t actively used.

The first step in providing strong isolation is setting up the kernel to run in its own address space.



#### `ptable.lock`

One invariant is that if a process is RUNNING, a timer interrupt’s yield must be able to switch away from the process; this means that the CPU registers must hold the process’s register values (i.e. they aren’t actually in a context), %cr3 must refer to the process’s pagetable, %esp must refer to the process’s kernel stack so that swtch can push registers correctly, and proc must refer to the process’s proc[] slot. Another invariant is that if a process is RUNNABLE, an idle CPU’s scheduler must be able to run it; this means that p->context must hold the process’s kerne thread variables, that no CPU is executing on the process’s kernel stack, that no CPU’s %cr3 refers to the process’s page table, and that no CPU’s proc refers to the process.

Once the code has started to modify a running process’s state to make it RUNNABLE, it must hold the lock until it has finished restoring the invariants: the earliest correct release point is after scheduler stops using the process’s page table and clears proc. Similarly, once scheduler starts to convert a runnable process to RUNNING, the lock cannot be released until the kernel thread is completely running (after the swtch, e.g. in yield).

##Code Threads

### include

#### `x86.h` (trap frame)

This file countains routines to  let C code use special x86 instructions.

**`trapftame()`**: layout of the trap frame built on the stack by the hardware and by `trapasm.S`, and passed to `trap()`. Trapframe stores register set which was saved during exception have arised, so using trapframe we can return back and proceed execution (when exception is handled).

Whenever control transfers into the kernel while a process is running, the hardware and xv6 trap entry code save user registers on the process’s kernel stack. These values are a `struct trapframe` which stores the user registers.

```C
struct trapframe {
  // registers as pushed by pusha
  uint edi;
  uint esi;
  uint ebp;
  uint oesp;      // useless & ignored
  uint ebx;
  uint edx;
  uint ecx;
  uint eax;

  // rest of trap frame
  ushort gs;
  ushort padding1;
  ushort fs;
  ushort padding2;
  ushort es;
  ushort padding3;
  ushort ds;
  ushort padding4;
  uint trapno;

  // below here defined by x86 hardware
  uint err;
  uint eip;
  ushort cs;
  ushort padding5;
  uint eflags;

  // below here only when crossing rings, such as from user to kernel
  uint esp;
  ushort ss;
  ushort padding6;
};
```

### kernel

#### `main.c` (main, mainc)



#### `defs.h` (irrelavent yet)

It contains definitions of many functions from many areas (e.g., bio, fs) internal to the kernel. One module, say fs, may want to use bio functions to read/write pages/blocks. In that case, fs will include defs.h so that it can call the bio functions. 

#### `proc.h` (proc, context)

Per-CPU variables, **holding pointers to the current cpu and to the current process**. The asm suffix tells gcc to use "%gs:0" to refer to cpu and "%gs:4" to refer to proc.  seginit sets up the %gs segment register so that %gs refers to the memory holding those two variables in the local cpu's struct cpu. This is similar to how thread-local variables are implemented in thread libraries such as Linux pthreads.

```C
extern struct cpu *cpu asm("%gs:0"); 
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc
```

**Two important structure**:

APICID: the processor’s unique hardware identifier.

```C
// Per-CPU state
struct cpu {
  uchar id;                    // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint booted;        // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

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

Saved registers for kernel context switches. Don't need to save all the segment registers (%cs, etc), because they are constant across kernel contexts. Don't need to save %eax, %ecx, %edx, because the x86 convention is that the caller has saved them.
Contexts are stored at the bottom of the stack they describe; the stack pointer is the address of the context. The layout of the context matches the layout of the stack in `swtch.S` at the "Switch stacks" comment. Switch doesn't save eip explicitly, but it is on the stack and allocproc() manipulates it.

The context is a set of callee-saved registers, which represents state of the task before it was preempted by other task (context switch).

```C
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;  // instruction pointer
};
```

> `struct context` is what gets saved when switching between two different execution points inside the kernel (like going from the timer interrupt to the scheduler or doing a context switch to kernel stack of switched process). the contents of `struct trapframe` are filled in when switching from user mode to kernel mode.  

####`vm.c`

##### switchuvm

Swicth page table using `switchuvm`, defined in `kernel/vm.c`. **h/w** means hardware. Switch TSS and h/w page table to correspond to process p.

**TSS**: the **task state segment** (TSS) is a special structure on x86-based computers which holds information about a task. It is used by the **operating system kernel** for task management. Specifically, the following information is stored in the TSS: 

- Processor register state
- I/O port permissions
- Inner-level stack pointers
- Previous TSS link

```C
void switchuvm(struct proc *p)
{
  pushcli();
  cpu->gdt[SEG_TSS] = SEG16(STS_T32A, &cpu->ts, sizeof(cpu->ts)-1, 0);
  cpu->gdt[SEG_TSS].s = 0;
  cpu->ts.ss0 = SEG_KDATA << 3;
  cpu->ts.esp0 = (uint)proc->kstack + KSTACKSIZE;
  ltr(SEG_TSS << 3);
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");
  lcr3(PADDR(p->pgdir));  // switch to new address space. Setting the page table base reg
  popcli();
}
```

##### switchkvm

Switch h/w page table register to the kernel-only page table, for when no process is running.

```C
void switchkvm(void) { lcr3(PADDR(kpgdir));}     // switch to the kernel page table 

// include/x86.h, setting page table base register.
static inline void lcr3(uint val) { asm volatile("movl %0,%%cr3" : : "r" (val));}
```



####`swtch.S` (contex switch)

Do context switch. The way to pass arguments to a function is by putting them on stack. Save current register context in old and then load register context from new.

`swtch`doesn’t directly know about threads; it just saves and restores register sets, called *contexts*. When it is time for a process to give up the CPU, the process’s **kernel thread** calls `swtch` to save its own context and **return to the scheduler context**.

Each context is represented by a `struct context*`, a pointer to a structure stored on the kernel stack involved.

```assembly
# void swtch(struct context **old, struct context *new);
# two parameters: context **old, context *new

.globl swtch
swtch:
  movl 4(%esp), %eax  # technically a volatile register, since the value isn't preserved.
  					  # parameter of swtch, which is context **old
  movl 8(%esp), %edx  # parameter of swtch, which is context *new
  					  # volatile general-purpose register that is occasionally used as a 					 	 function parameter. 

  # Save old callee-save registers, because they are non-volatile, function should    		preserve their values are un-changed
  pushl %ebp  # non-volatile. Either the frame pointer or a general purpose register.
  pushl %ebx  # non-volatile general-purpose register. It has no specific uses.
  pushl %esi  # non-volatile general-purpose register that is often used as a pointer. 					Often stores data that's used throughout a function because it doesn't 					change. esi points to the "source". 
  pushl %edi  # non-volatile general-purpose register that is often used as a pointer. 					Generally used as a destination for data.

  # Switch stacks
  movl %esp, (%eax)  # implicitly saved as the struct context written to old
  				  	 # store address of %esp to context *old (which is also an address).
  				  	 # so that when restore, knows the address of old's stack frame. 						   (Where non-volatile registers stores)
  movl %edx, %esp  # context of new, swithc %esp to it.

  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret

```

#### `trap.c`



#### `proc.c`

##### allocproc

`allocproc` look in the process table for an `UNUSED` proc. If found, change state to `EMBRYO` and initialize state required to run in the kernel. Otherwise return 0.

```C
static struct proc* allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack if possible. If cannot allocate, process will be set UNUSED.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;    // include/param.h:  #define KSTACKSIZE 4096
  // KSTACKSIZE defined in include/param.h, size of per-process kernel stack. 
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret, which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}
```

The kernel thread will start executing with register contents copied from `p->context`. 

##### forkret

Thus setting `p->context->eip` to forkret will cause the kernel thread to execute at the start of forkret. **This function will return to whatever address is at the bottom of the stack**. The context switch code sets the stack pointer to point just beyond the end of `p->context`. `allocproc` places `p->context` on the stack, and puts a pointer to trapret just above it; that is where forkret will return. **`trapret` restores user registers from values stored at the top of the kernel stack and jumps into the process**.

```C
// A fork child's first scheduling by scheduler() will swtch here. Return to user space.
void forkret(void)
{ // Still holding ptable.lock from scheduler.
  release(&ptable.lock); 
  // Return to "caller", actually trapret (see allocproc).
}
```

#####  fork (131)

`fork()` create a new process copying p as the parent. Sets up stack to return as if from system call. **Caller must set state of returned proc to `RUNNABLE`. **

```C
int fork(void)
{
  int i, pid;
  struct proc *np;
  									// Allocate process. *allocproc* will find a slot in 
  if((np = allocproc()) == 0)		// ptable and set up kernel stack, trap frame, context
    return -1;						// for this new baby process.

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  np->tf->eax = 0;  // Clear %eax so that fork returns 0 in the child.

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}
```

##### wait

In wait, the parent process will clean the `ZOMBIE` child process.

```C
int wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){ 							// Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){   	// If found one of its zombie child
        pid = p->pid;
        // ... 						// Set this slot of proc to be unused, free kstack and
        release(&ptable.lock);		// pgdir of child proc
        return pid;					// Return this child's pid
      }
    }
    if(!havekids || proc->killed){  // if no child or current proc has been killed, don't 
      release(&ptable.lock);		// need to wait, return -1
      return -1;
    }
    sleep(proc, &ptable.lock);  	// If child not zombie, go to sleep
  }
}
```

##### exit

Exit current process. Stay in `ZOMBIE` until its parent finds it exited in `wait()`.

```C
void exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  proc->state = ZOMBIE; // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}
```

##### yield

`yield()` give up the CPU for one scheduling round.

```C
void yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}
```

##### sched

`sched` called `swtch` to switch to `cpu->scheduler`, the per-CPU scheduler context. `sched` enter scheduler.  Must hold only `ptable.lock` and have changed `proc->state`.

```C
void sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}
```

##### scheduler

Per-CPU process scheduler. Each CPU calls `scheduler()` after setting itself up. Scheduler never returns.  It loops, doing:

- choose a process to run
- swtch to start running that process
- eventually that process transfers control via `swtch` back to the scheduler.

Timer interrupt is raises by h/w, and os just handles it.

```C
void scheduler(void)
{
  struct proc *p;

  for(;;){
    sti();  // Enable interrupts on this processor.

    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);   // jump out of the scheduler context, enter
      // the proc context. After swtch, not in scheduler anymore. Goes to proc context.  
      // Need to come back. Way to come back: timer interrupt (trap), which calls yield().
      switchkvm();   /// when back to scheduler context, restore registers.
        
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);

  }
}
```

##### userinit

Set up first user process. `userinit` calls `setupkvm` to create a page table for the process with (at first)
mappings only for memory that the kernel uses. 

```C
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE; //  stack ptr, set to the process’s largest valid virtual address
  p->tf->eip = 0;  // instruction pointer; beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}
```

#### `trapasm.S`

The four are all registers in the trap frame. Values being poped off the stack into them, restore user mode.

```assembly
  # Return falls through to trapret...
.globl trapret
trapret:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret
```

#### `initcode.S`

The first program to be executed by the first process. It stores the parameters needed for the first **user program "init"**,  and then calls the system call "sys_exec", which then checks a couple of things, and then call `exec()` defined in the `kernel/exec.c`.

```assembly
.globl start
start:
  pushl $argv
  pushl $init
  pushl $0  // where caller pc would be
  movl $SYS_exec, %eax
  int $T_SYSCALL    # it calls the exec system call to execute the program "init"

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0
```

#### `exec.c`

Every time a new child process is forked, it has the same program name as its parent. Only when` exec` begins to execute the called program, its name will then change.

```C
int exec(char *path, char **argv) {
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;

  if((ip = namei(path)) == 0)
    return -1;
  ilock(ip);
  pgdir = 0;

  // Check ELF header. If error, go to bad.
  // Load program into memory.
  // Allocate a one-page stack at the next page boundary
  // Push argument strings, prepare rest of stack in ustack...
    
  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc->name, last, sizeof(proc->name));

  // Commit to the user image...
  return 0;

 bad: //... 
  return -1;
}
```

### user

#### `init.c`

This program is the first user program, used to call `sh`. `fork` makes the new child process runnable, and add to the cpu's process table.

```C
int main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){                                     // Endless for loop. If EVER have a chance
    printf(1, "init: starting sh\n");          // to repeat the loop, means sh has exit
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();								   // fork(), pid became 2, at first has same
    } 										   // process name as its parent
    if(pid == 0){                              // From this child process, will entre sh
      exec("sh", argv);						   // If no error, will exit in sh
      printf(1, "init: exec sh failed\n");	   // the child process first names "init", 
      exit();								   // then when sh starts, names "sh"
    }
    while((wpid=wait()) >= 0 && wpid != pid)   // wait is in parent process. If it caught
      printf(1, "zombie!\n");                  // one of its child, return that child's 
  }											   // pid. If no child, return -1. >= 0 means
}											   // some child program returns.
```

& means let a project runs in the background.

```sh
process chosen to run init, pid 2
process chosen to run init, pid 2
process chosen to run sh, pid 2
$ ls &; ls
process chosen to run sh, pid 2
process chosen to run sh, pid 3		# pid=3 is  running in the background, and since there
process chosen to run sh, pid 3 	# is only one cpu, it wait for pid=5 to run first
process chosen to run sh, pid 4
process chosen to run sh, pid 5
process chosen to run sh, pid 3
process chosen to run sh, pid 5
... # omit run pid 5 parts here
process chosen to run sh, pid 5		# it takes time for sh to get ls execute. 
process chosen to run sh, pid 3		# at the time when pid=5 is exec, 3 gets chance to run
process chosen to run ls, pid 5		# now 5 starts to write to stdout, => block
.              1 1 512
..             1 1 512
README         2 2 1793
cat            2 3 9448
echo           2 4 8992
forktest       2 5 5784
grep           2 6 10568
init           2 7 9292
process chosen to run sh, pid 3
process chosen to run ls, pid 5
kill           2 8 9008
process chosen to run sh, pid 3
process chosen to run ls, pid 5
ln             2 9 8984
process chosen to run sh, pid 3		# when pid 5 is dealing with output, 3 could run!
process chosen to run sh, pid 3
process chosen to run sh, pid 3
process chosen to run sh, pid 3
process chosen to run sh, pid 3
process chosen to run sh, pid 3
.              1 1 512
process chosen to run ls, pid 5
ls             2 10 10556
mkdir          2 11 9076
rm             2 12 9056
sh             2 13 16180
spin           2 14 8964
stressfs       2 15 9216
process chosen to run ls, pid 3
..             1 1 512
process chosen to run ls, pid 5
tprocess chosen to run ls, pid 3
README         2 2 1793
cat            2 3 9448
echo           2 4 8992
forktest       2 5 5784
grep           2 6 10568
init           2 7 9292
kill           2 8 9008
ln             2 9 8984
ls             2 10 10556
mkdir          2 11 9076
process chosen to run ls, pid 5
ester         2 16 8928
usertests      2 17 34828
wc             2 18 9780
zombie         2 19 8784
console        3 20 0
process chosen to run init, pid 1
zombie!
process chosen to run ls, pid 3
rm             2 12 9056
process chosen to run ls, pid 3
sh             2 13 16180
spin           2 14 8964
stressfs       2 15 9216
tester         2 16 8928
usertests      2 17 34828
wc             2 18 9780
zombie         2 19 8784
console        3 20 0
process chosen to run sh, pid 2
process chosen to run sh, pid 2
$ 
```





#### `sh.c`

The first user program calls `sh`, which will continue to run command programs.

```C
int main(void)
{
  // ...
  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){  // while there is input
    // ...
    if(fork1() == 0)   // fork1() will call fork() to run the command && if in child,
      runcmd(parsecmd(buf));
    wait();            // if in main, wait for this child of sh to exit.
  }
```

`runcmd` will execute the program in the child process, and then exit.

```C
void
runcmd(struct cmd *cmd)
{
  // ...
  switch(cmd->type){
  default: // ...
  case ...
  }
  exit();
}
```



## Write system call

In user mode, the write system function is declared as

```C
int write(int, void*, int);

int sys_write(void)
{
  // ...
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}
```

To handle the system call with arguments, other kernel functions are needed to access and parse the parameters. `write` utilizes three functions. The first one deals with a file descriptor, which we do not need in this project.

```C
// sysfile.c
// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int argfd(int n, int *pfd, struct file **pf);
```

The second and third one are defined in `syscall.c`.

```C
// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip)
{
  return fetchint(proc, proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the int at addr from process p.
int fetchint(struct proc *p, uint addr, int *ip)
{
  if(addr >= p->sz || addr+4 > p->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nth word-sized system call argument as a pointer to a block of memory of size 
// n bytes.  Check that the pointer lies within the process address space.
int argptr(int n, char **pp, int size)
{
  int i;
  
  if(argint(n, &i) < 0)
    return -1;
  if((uint)i >= proc->sz || (uint)i+size > proc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

```