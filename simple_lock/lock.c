/*
int FetchAndAdd(int *ptr) {
	int old = *ptr;
	*ptr = old + 1;
	return 0ld;
}

*/

#include <stdio.h>

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

typedef struct __lock_t {
	int ticket;
	int turn;
} lock_t;

void lock_init(lock_t *lock) {
	lock->ticket = 0;
	lock->turn = 0;
}

void lock(lock_t *lock) {
	int myturn = fetch_and_add(&lock->ticket);
	while (lock->turn != myturn)
		;
}

void unlock(lock_t *lock) {
	lock->turn = lock->turn + 1;
}

int main() {
	int a = 6;
	int b = fetch_and_add(&a);
	printf("a %d, b %d\n", a, b);
	return 0;
}




























