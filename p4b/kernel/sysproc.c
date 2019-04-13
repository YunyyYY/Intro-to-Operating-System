#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_dummy(void)
{
  //cprintf("I am dummy\n");
  struct proc *p;
  cprintf("proc: pid: %d, size: %x, stack: %x, pc: %x\n",
          proc->pid, proc->sz, proc->tf->esp, proc->tf->eip);
  p = proc->parent;
  cprintf("parent: pid: %d, size: %x, stack: %x, pc: %x\n",
          p->pid, p->sz, p->tf->esp, p->tf->eip);
  return 0;
}


int
sys_clone(void)
{
  // first parse arguments
  void(*f) (void *, void *);
  void *fn;
  void *arg1, *arg2, *stack; // (address of stack)
  if (argvoid(0, &fn)<0 || argvoid(1, &arg1)<0 ||
    argvoid(2, &arg2)<0 || argvoid(3, &stack)<0)
    return -1;
  // cprintf("sysproc.c:21 %d %d\n", *(int* )arg1, *(int* )arg2);
  f = (void (*)(void *, void *))fn;
  // cprintf("sysproc.c:35 parsed %x\n", (uint)stack);
  return clone(f, arg1, arg2, stack);
}

int
sys_join(void)
{
  void *stack;
  if (argvoid(0, &stack)<0)
    return -1;
  stack = (void** )stack;
  return join(stack);
}

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
