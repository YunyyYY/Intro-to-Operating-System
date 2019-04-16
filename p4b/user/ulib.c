#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define SSIZE 64

struct stack_pair {
    void* start;
    void* aligned;
};

struct stack_pair stable[SSIZE];

int
thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)
{

  void *stack = malloc(2 * MYPAGE);
  if (stack == 0) { printf(1, "[thread_create] malloc not enough memory\n");
    return -1;}

  int idx  = 0;
  for (int i = 0; i < SSIZE; i++) {
    if (stable[i].start == 0){
      idx = i;
      stable[i].start = stack;
      stable[i].aligned = (void *)((uint)(stack + MYPAGE -1) & ~(MYPAGE -1));
      stack = stable[i].aligned;
      break;
    }
  }

  int pid = clone(start_routine, arg1, arg2, stack);

  if (pid == -1){
    free(stable[idx].start);
    stable[idx].start = stable[idx].aligned = 0;
    return -1;
  }

  if (pid == 0) {
    printf(1, "pid == 0, stack at %x\n", stack);
    dummy();
    return -1;
  }
  return pid;
}

int
thread_join()
{
  void* stack = 0;
  int pid = join(&stack);

  if (stack != 0) {
    for (int i = 0; i < SSIZE; i++){
      if (stack == stable[i].aligned){
        free(stable[i].start);
        stable[i].aligned = stable[i].start = 0;
        break;
      }
    }
  }

  return pid;
}

static inline int fetch_and_add(int* variable)
{
  int value = 1;
  asm volatile("lock; xaddl %0, %1"
  : "+r" (value), "+m" (*variable) // input+output
  : // No input-only
  : "memory"
  );
  return value;
}

void
lock_init(lock_t * lock)
{
  lock->ticket = 0;
  lock->turn = 0;
}

void
lock_acquire(lock_t * lock)
{
  int myturn = fetch_and_add(&lock->ticket);
  while (lock->turn != myturn)
    ;
}

void
lock_release(lock_t * lock)
{
  lock->turn = lock->turn + 1;
}

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}
