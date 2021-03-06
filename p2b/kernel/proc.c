#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "pstat.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct {
    struct spinlock lock;
    int level;
} plevel;

struct {
    struct spinlock lock;
    struct proc *zero[NPROC];
    int first;
    int size;
} zque;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

int max_level[] = {1000, 32, 16, 8};
int max_wait[] = {500, 320, 160, 0};

static void wakeup1(void *chan);

void
get_pstat(struct pstat *ps)
{
  int idx = 0;
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->state == UNUSED)
      continue;
    // p is a process in use
    while (ps->inuse[idx] != 0)
      idx++;
    ps->inuse[idx] = 1;
    ps->pid[idx] = p->pid;
    ps->priority[idx] = p->priority;
    ps->state[idx] = p->state;
    for (int i = 0; i < NLAYER; i++){
      ps->ticks[idx][i] = p->ticks[i];
      ps->wait_ticks[idx][i] = p->wait_ticks[i];
    }
    if (ps->ticks[idx][3] == 0)
      ps->ticks[idx][3] = 1;
  }
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

void
levinit(void)
{
  initlock(&plevel.lock, "plevel");
  initlock(&zque.lock, "zque");
}

// delete the process from the zero priority queue.
void
deque(struct proc* p)
{
  int i0;
  for (i0 = 0;i0< zque.size;i0++) {
    if(zque.zero[(i0+zque.first)%NPROC]==p)
      break;
  }
  acquire(&zque.lock);
  for (int k = i0; k < zque.size; k++)
    zque.zero[(k+zque.first)%NPROC] = zque.zero[(k+zque.first+1)%NPROC];
  zque.size--;
  release(&zque.lock);
}

//
void
update_tick(void)
{
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE) {
      p->wait_ticks[p->priority]++;
      if((p->priority<3) && (p->wait_ticks[p->priority]
                             == max_wait[p->priority])){
        p->wait_ticks[p->priority ]=0;
        // p->ticks[p->priority]=0;
        if (p->priority==0)  // move from zero queue
          deque(p);
        p->priority++;
      }
    }
  }
  release(&ptable.lock);
  proc->ticks[proc->priority]++;
  if((proc->ticks[proc->priority] > 0) &&
          (proc->ticks[proc->priority]%max_level[proc->priority] == 0)){
    if (proc->priority > 0)
      proc->priority--;
  }
  if (proc->priority == 0) {  // add to zero queue
    zque.zero[(zque.first+zque.size)%NPROC] = proc;
    zque.size++;
  }
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
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

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
void
userinit(void)
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
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->priority = 3;

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

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

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));

  np->priority = 3;
  for (int i = 0; i < NLAYER; i++){
    np->ticks[i] = 0;
    np->wait_ticks[i] = 0;
  }

  acquire(&plevel.lock);
  plevel.level = 3;
  release(&plevel.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
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

  if(proc->priority==0){
    deque(proc);
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
update_level()
{
  int level = 0;
  struct proc *pl;
  for(pl = ptable.proc; pl < &ptable.proc[NPROC]; pl++){
    if(pl->state != RUNNABLE)
      continue;
    if(pl->priority > level)
      level = pl->priority;
  }
  acquire(&plevel.lock);
  plevel.level = level;
  release(&plevel.lock);
  return level;
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p, *pprev=initproc;

  for(;;){
    sti();    // Enable interrupts on this processor.

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    update_level();

    // if current process runable
    if((pprev->state == RUNNABLE) && (pprev->priority == plevel.level) &&
      (pprev->ticks[pprev->priority]<max_level[pprev->priority])){
      proc = pprev;
      // cprintf("\nflag 0: run current process!\n");

      switchuvm(proc);
      proc->state = RUNNING;
      //cprintf("process chosen to run %s, pid %d\n", proc->name, proc->pid);
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      proc = 0;
      release(&ptable.lock);
      continue;
    }

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      update_level();  // update current highest priority


      if (plevel.level == 0){
        for (int i0 = 0; i0 < zque.size; i0++){
          if(zque.zero[(i0+zque.first)%NPROC]->state == RUNNABLE) {
            proc = zque.zero[(i0+zque.first)%NPROC];
            break;
          }
        }
        // cprintf("\nflag 2: plevel = 0\n");
      } else {
        if(p->priority < plevel.level)
          continue;
      }

      // cprintf("\nflag 3: %s, %d\n", p->name, plevel.level);

      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      //cprintf("process chosen to run %s, pid %d\n", p->name, p->pid);
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      pprev = p;
      proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
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

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


