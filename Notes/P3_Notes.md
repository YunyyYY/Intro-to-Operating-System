# P3 Notes

## Mechanisms

### xv6 PageTable

When a process asks xv6 for more memory, xv6 first finds free physical pages to provide the storage, and then adds PTEs to the process’s page table that point to the new physical pages. Different processes’ page tables translate user addresses to different pages of physical memory, so that each process has private user memory.

In the virtual address space of every process, the kernel code and data begin from `KERNBASE` (2GB in the code), and can go up to a size of `PHYSTOP` (whose maximum value can be 2GB). **This virtual address space of [KERNBASE, KERNBASE+PHYSTOP] is mapped to [0,PHYSTOP] in physical memory**. The kernel is mapped into the address space of every process, and the kernel has a mapping for all usable physical memory as well, restricting xv6 to using no more than 2GB of physical memory.



### [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) or the Executable and Linkable Format

ELF is the abbreviation for **Executable and Linkable Format** and defines the structure for binaries, libraries, and core files. The formal specification allows the operating system to interprete its underlying machine instructions correctly. ELF files are typically the output of a compiler or linker and are a binary format.

#### `inode`

An inode is a data structure that stores the metadata (such as name, type, size) of a on-disk file. Basically the `exec()` function uses `inode` to **locate and read the program data from binary file** that is specified by the `path` argument. 

### Trap Frame

Each handler sets up a trap frame and then calls the C function trap. Trap looks at the hardware trap number `tf->trapno` to decide why it has been called and what needs to be done. If the trap is `T_SYSCALL`,
trap calls the system call handler syscall. If the trap is not a system call and not a hardware device looking for attention, trap assumes it was caused by incorrect behavior (e.g., divide by zero) as part of the code that was executing before the trap. If the code that caused the trap was a user program, xv6 prints details and then sets `cp->killed` to remember to clean up the user process.  

### ELF Program Headers: MemSiz vs. FileSiz

“File size” field tells us how many bytes long the segment is in the file. 

memsize $\geq​$ filesize.

### Stack grows from larger address to lower address

```sh
$ p3_very_long_long_name
exec 55: after loading binary size is 2812
exec 59: after roundup binary size is 4096
exec 84: after *** stack pointer is 5fd4
stack: start address of a 5FA8
stack: end address of a 5FB7
heap: address of b DFF0
stack: start address of a 5F98
stack: end address of a 5FA7
$ p3
exec 55: after loading binary size is 2812
exec 59: after roundup binary size is 4096
exec 84: after *** stack pointer is 5fe8
stack: start address of a 5FB8
stack: end address of a 5FC7
heap: address of b DFF0
stack: start address of a 5FA8
stack: end address of a 5FB7
heap: address of b DFD8
```



## C norms

### 2 ways to define function-like macros

1. define with function-like bodies and escape char "\\"

   ```c
   #define SETGATE(gate, istrap, sel, off, d)                \
   {                                                         \
     (gate).off_15_0 = (uint)(off) & 0xffff;                \
     (gate).cs = (sel);                                      \
     (gate).args = 0;                                        \
     (gate).rsv1 = 0;                                        \
     (gate).type = (istrap) ? STS_TG32 : STS_IG32;           \
     (gate).s = 0;                                           \
     (gate).dpl = (d);                                       \
     (gate).p = 1;                                           \
     (gate).off_31_16 = (uint)(off) >> 16;                  \
   }
   
   ```

   > This function is defined in mmu.h, where `gate` is expected to be `struct gatedesc`.

2. define with bracket

   ```C
   #define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
   ```

### `void*`

A void pointer is a pointer that has no associated data type with it. A void pointer can hold address of any type and can be typcasted to any type. `malloc()` and `calloc()` return `void *` type and this allows these functions to be used to allocate memory of any data type. void pointers in C are used to implement **generic functions** in C.

**NOTICE**

- void pointers cannot be dereferenced. 
- A  void pointer can be dereferenced if it is dereferenced with typecast in front of it.

### `for loop`

There can be multiple instructions in C.

```C
for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph))
```

The following condition here is same as `ptr != NULL`:

```C
for(argc = 0; argv[argc]; argc++)
```

### `union`

```C
typedef union header Header;
```

A **union** is a special data type available in C that allows to store different data types in the same memory location. You can define a union with many members, but only one member can contain a value at any given time. Unions provide an efficient way of using the same memory location for multiple-purpose.

### `asm` in C

You can specify the name to be used in the assembler code for a C function or variable by writing the `asm` (or `__asm__`) keyword after the declarator. It is up to you to make sure that the assembler names you choose do not conflict with any other assembler symbols, or reference registers.

```C
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
```

This code is using a gcc extension called [asm labels](https://gcc.gnu.org/onlinedocs/gcc/Asm-Labels.html#Asm-Labels) to say that the variable `cpu` is defined by the assembler string `%gs:0`. This is NOT how this extension is intended to be used and is considered a hack.

#### Assembler Instructions with C Expression Operands

In an assembler instruction using `asm`, you can specify the operands of the instruction using C expressions. This means you need not guess which registers or memory locations will contain the data you want to use.

You must specify an assembler instruction template much like what appears in a machine description, plus an operand constraint string for each operand.

`volatile` means no optimization is needed.

AssemblerTemplate is a combination of fixed text and tokens that refer to the input, output, and goto parameters. 

- `cld`: clear direction flag so that string pointers auto increment after each string operation
- `in` transfers a byte, word, or long from the immediate port into the byte, word, or long memory address pointed to by the AL, AX, or EAX register, respectively.
- `rep`: The index register is incremented if DF = 0 (DF cleared by a cld instruction); it is decremented if DF = 1 (DF set by a stdinstruction). The increment or decrement count is 1 for a byte transfer, 2 for a word, and 4 for a long. Use the rep prefix with the insinstruction for a block transfer of CX bytes or words.
- `insl`: Transfer a string from the **port address, specified in the DX register**, into the **ES:destination index register**

OutputOperands: comma-separated list of the C variables modified by the instructions in the AssemblerTemplate.

InputOperands: comma-separated list of C expressions read by the instructions in the AssemblerTemplate.

Clobbers:  two special clobber arguments:

- The `"cc"` clobber indicates that the assembler code modifies the flags register.
- The `"memory"` clobber tells the compiler that the assembly code performs memory reads or writes to items other than those listed in the input and output operands (for example, accessing the memory pointed to by one of the input parameters). 

```C
asm volatile("cld; rep insl" :						// AssemblerTemplate
               "=D" (addr), "=c" (cnt) :			// OutputOperands
               "d" (port), "0" (addr), "1" (cnt) :	// InputOperands
               "memory", "cc");						// Clobbers
```

The `=` indicates that the operand is an output; all output operands' constraints must use `=`, which means write-only.

## Code Thread

### `include/x86.h`

Layout of the trap frame built on the stack by the hardware and by `trapasm.S`, and passed to `trap()`. The way that control transfers from user software to the kernel is via an interrupt mechanism, which is used by system calls, interrupts, and exceptions. Whenever control transfers into the kernel while a process is running, the hardware and xv6 trap entry code save user registers on the process’s kernel stack. These values are a `struct trapframe` which stores the user registers.

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

### `kernel/mmu.h`

```C
// Gate descriptors for interrupts and traps
struct gatedesc {
  uint off_15_0 : 16;   // low 16 bits of offset in segment
  uint cs : 16;         // code segment selector
  uint args : 5;        // # args, 0 for interrupt/trap gates
  uint rsv1 : 3;        // reserved(should be zero I guess)
  uint type : 4;        // type(STS_{TG,IG32,TG32})
  uint s : 1;           // must be 0 (system)
  uint dpl : 2;         // descriptor(meaning new) privilege level
  uint p : 1;           // Present
  uint off_31_16 : 16;  // high bits of offset in segment
};
```



### `kernel/elf.h`

```C
// Program section header
struct proghdr {
  uint type;	// how to interpret the array element's information
  uint offset;	// offset from beginning of file where the first byte of segment resides.
  uint va;		// virtual address at which the first byte of the segment resides in memory.
  uint pa;		// segment's physical addr for systems when physical addressing is relevant.
  uint filesz;	// number of bytes in the file image of the segment, which can be zero.
  uint memsz;	// number of bytes in the memory image of the segment, which can be zero
  uint flags;	// Flags relevant to the segment.
  uint align;	// Loadable process segments must have congruent values for p_vaddr and 
    			// p_offset, modulo the page size. This member gives the value to which the
    			// segments are aligned in memory and in the file. 
};

```



### `kernel/main.c`

```C
int main(void)
{
  mpinit();        // collect info about this machine
  lapicinit(mpbcpu());
  seginit();       // set up segments
  kinit();         // initialize memory allocator
  jmpkstack();       // call mainc() on a properly-allocated stack 
}
```



### `kernel/trap.c`

```C
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
```

```C
SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
```



### `kernel/kalloc.c`

The allocator’s data structure is a *free list* of physical memory pages that are available for allocation. Each free page’s list element is a `struct run`.

```c
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

extern char end[]; // first address after kernel loaded from ELF file

// Initialize free list of physical pages.
void kinit(void)
{
  char *p;

  initlock(&kmem.lock, "kmem");
  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE)
    kfree(p);
}
```

`kfree()` free the page of physical memory pointed at by v, which normally should have been returned by a call to `kalloc()`.  (The exception is when initializing the allocator; see `kinit` above.)

```C
void kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}
```

Allocate one 4096-byte page of physical memory. Returns a pointer that the kernel can use. Returns 0 if the memory cannot be allocated.

```C
char* kalloc(void){							
  struct run *r;			
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);
  return (char*)r;
}
```



### `kernel/proc.c`

```C
// Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)													// (*)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);			// (**)
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));													// (***)
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}
```



### `kernel/fs.c`

Read data from inode. It starts making sure that the offset and count are not reading beyond the end of the file. The main loop processes each block of the file, copying data from the `buffer` into `dst`.

```C
int readi(struct inode *ip, char *dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
      return -1;
    return devsw[ip->major].read(ip, dst, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > ip->size)
    n = ip->size - off;

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(dst, bp->data + off%BSIZE, m);
    brelse(bp);
  }
  return n;
}

```





### `kernel/vm.c`

`pde_t` is defined in `include/types.h`, and is just a data type.

```C
typedef uint pde_t;
```

The kernel allocates memory for its heap and for **user memory** between kernend and the end of physical memory (PHYSTOP). The virtual address space of each user program includes the kernel (which is inaccessible in user mode).  The user program addresses range from 0 till 640KB (USERTOP), which where the I/O hole starts (both in physical memory and in the kernel's virtual address space).

```C
static struct kmap {
  void *p;
  void *e;
  int perm;
} kmap[] = {
  {(void*)USERTOP,    (void*)0x100000, PTE_W},  // I/O space
  {(void*)0x100000,   data,            0    },  // kernel text, rodata
  {data,              (void*)PHYSTOP,  PTE_W},  // kernel data, memory
  {(void*)0xFE000000, 0,               PTE_W},  // device mappings
};
```

Here 0 is the top of the the device mapping part, this is because

```C
0 - 0xFE000000 = 0x2000000
```

So `0` can be viewed as `0x100000000`, which is the top of the stack (larger than `0xffffffff`).

Next is `walkpgdir`.

Return the address of the **PTE in page table pgdir** that corresponds to **linear address va**.  If `create!=0`, create any required page table pages.

```C
static pte_t *walkpgdir(pde_t *pgdir, const void *va, int create)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)PTE_ADDR(*pde);
  } else {
    if(!create || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table 
    // entries, if necessary.
    *pde = PADDR(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}
```

Set up the kernel part of a page table:

```C
pde_t* setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  k = kmap;
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->p, k->e - k->p, (uint)k->p, k->perm) < 0)
      return 0;

  return pgdir;
}
```

`memset()` is an intersting function defined in `kernel/string.c`.

```C
void* memset(void *dst, int c, uint n) {
  stosb(dst, c, n);
  return dst;
}
```

It points to an even more intersting function `stosb()` defined in `include/x86.h`

```C
static inline void stosb(void *addr, int data, int cnt) {
  asm volatile("cld; rep stosb" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}
// addr: pgdir, data: 0, cnt: PGSIZE
```

This is some assmbler inline instruciton.

Create PTEs for linear addresses starting at `la` that refer to physical addresses starting at `pa`. `la` and size might not be page-aligned.

```C
static int mappages(pde_t *pgdir, void *la, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;
  
  a = PGROUNDDOWN(la);
  last = PGROUNDDOWN(la + size - 1);
  for(;;){
    pte = walkpgdir(pgdir, a, 1);
    if(pte == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}
```

line 129-137 defined page table/directory entry flags.

```C
// Page table/directory entry flags.
#define PTE_P		0x001	// Present
#define PTE_W		0x002	// Writeable
#define PTE_U		0x004	// User
#define PTE_PWT		0x008	// Write-Through
#define PTE_PCD		0x010	// Cache-Disable
#define PTE_A		0x020	// Accessed
#define PTE_D		0x040	// Dirty
#define PTE_PS		0x080	// Page Size
#define PTE_MBZ		0x180	// Bits must be zero
```

Allocate page tables and physical memory to grow process from oldsz to newsz, which need not be page aligned.  Returns new size or 0 on error.

```C
int allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz > USERTOP)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();		// allocate a page
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    mappages(pgdir, (char*)a, PGSIZE, PADDR(mem), PTE_W|PTE_U);
  }
  return newsz;
}
```

Load a program segment into pgdir. `addr` must be page-aligned and the pages from addr to addr+sz must already be mapped.

```C
int loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint)addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)	// find the pte of each entry in pgdir
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);				// #define PTE_ADDR(pte)	((uint)(pte) & ~0xFFF)
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, (char*)pa, offset+i, n) != n)	// load instructions from <inode> to <pa>.
      return -1;
  }
  return 0;
}
```

Given a parent process's page table,create a copy of it for a child.

```C
pde_t* copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void*)i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))							// #define PTE_P		0x001	// Present
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, PADDR(mem), PTE_W|PTE_U) < 0)
      goto bad;
  }
  return d;

bad:
  freevm(d);
  return 0;
}
```



### `kernel/exec.c`

`exec` first check if the 

```C
int exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];		// include/param.h:17:#define MAXARG 32
    											// 		max exec arguments
  struct elfhdr elf;							// elf.h:8:File header
  struct inode *ip;								// file.h:16:in-core file system types
  struct proghdr ph;							// elf.h:27:Program section header
  pde_t *pgdir, *oldpgdir;

  if((ip = namei(path)) == 0)					// struct inode* namei(char *path)
    return -1;									// if ip==0, means return inode fail
  ilock(ip);									// Lock the given inode.
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))	// something like getline()
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.va + ph.memsz)) == 0)   // allocate memory for program
      goto bad;    // ph.va = 0x0 is virtual address ph.memsz is total size of ph
    if(loaduvm(pgdir, (char*)ph.va, ip, ph.offset, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  ip = 0;

  // Allocate a one-page stack at the next page boundary
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + PGSIZE)) == 0)
    goto bad;

  // Push argument strings, prepare rest of stack in ustack.
  sp = sz;
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp &= ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc->name, last, sizeof(proc->name));

  // Commit to the user image.
  oldpgdir = proc->pgdir;
  proc->pgdir = pgdir;
  proc->sz = sz;
  proc->tf->eip = elf.entry;  // main
  proc->tf->esp = sp;
  switchuvm(proc);
  freevm(oldpgdir);

  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip)
    iunlockput(ip);
  return -1;
}
```

### `user/makefile.mk`

```reStructuredText
$@			The file name of the target of the rule.  If the target is an archive member, 				then ‘$@’ is the name of the archive file.
$%			The target member name, when the target is an archive member. 
			For example, if the target is foo.a(bar.o) then ‘$%’ is bar.o and ‘$@’ is foo.a.
$<			The name of the first prerequisite. 
```

*Archive files* are files containing named sub-files called *members*; they are maintained with the program `ar` and their main use is as subroutine libraries for linking.

```C
# location in memory where the program will be loaded
USER_LDFLAGS += --section-start=.text=0x0
    
# user programs
user/bin/%: user/%.o $(USER_LIBS) | user/bin
	$(LD) $(LDFLAGS) $(USER_LDFLAGS) --output=$@ $< $(USER_LIBS)
```

### `user/sh.c`

Line 328 ~ 343:

```C
struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}
```



## Implement

### A program without `exit()`

```C
// trap.c

cprintf("pid %d %s: trap %d err %d on cpu %d eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip, 
            rcr2());
```

```sh
pid 4 p3: trap 14 err 5 on cpu 0 eip 0xffffffff addr 0xffffffff--kill proc
# In gdb mode, the following info is found:
(gdb) p tf->trapno
$1 = 14
```

Trapno 14 indicates a page fault.

```C
// include/traps.h
#define T_PGFLT         14      // page fault
```



## Visualization

### Kernel Stack

trapframe里的eip正是进入用户态之后执行的程序的入口，init进程跟普通进程的差别就在这里了，init进程设置的eip=0，在此之前0这里已经放置了initcode.S的内容，而普通进程这里设置的是elf->entry，即程序的main()函数。

```reStructuredText
Initial setup of the kernel stack:

                  /   +---------------+ <-- stack base(= p->kstack + KSTACKSIZE)
                  |   | ss            |                           
                  |   +---------------+                           
                  |   | esp           |                           
                  |   +---------------+                           
                  |   | eflags        |                           
                  |   +---------------+                           
                  |   | cs            |                           
                  |   +---------------+                           
                  |   | eip           | <-- When iret automatically pop into correspongding
                  |   +---------------+     register. just let %esp points here
                  |   | err           |  
                  |   +---------------+  
                  |   | trapno        |  
                  |   +---------------+                       
                  |   | ds            |                           
                  |   +---------------+                           
                  |   | es            |                           
                  |   +---------------+                           
                  |   | fs            |                           
 struct trapframe |   +---------------+                           
                  |   | gs            |                           
                  |   +---------------+   
                  |   | eax           |   
                  |   +---------------+   
                  |   | ecx           |   
                  |   +---------------+   
                  |   | edx           |   
                  |   +---------------+   
                  |   | ebx           |   
                  |   +---------------+                        
                  |   | oesp          |   
                  |   +---------------+   
                  |   | ebp           |   
                  |   +---------------+   
                  |   | esi           |   
                  |   +---------------+   
                  |   | edi           |   
                  \   +---------------+ <-- p->tf                 
                      | trapret       |                           
                  /   +---------------+ <-- forkret will return to
                  |   | eip(=forkret) | <-- return addr           
                  |   +---------------+                           
                  |   | ebp           |                           
                  |   +---------------+                           
   struct context |   | ebx           |                           
                  |   +---------------+                           
                  |   | esi           |                           
                  |   +---------------+                           
                  |   | edi           |                           
                  \   +-------+-------+ <-- p->context            
                      |       |       |                           
                      |       v       |                           
                      |     empty     |                           
                      +---------------+ <-- p->kstack           
```



## Reference

1. http://ybin.cc/os/xv6-init-process/