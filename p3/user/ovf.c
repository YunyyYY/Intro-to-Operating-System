#include "types.h"
#include "stat.h"
#include "user.h"

// according to test, 955 work and 956 fails.

int
main(int argc, char *argv[])
{
    if (argc < 2)
        exit();
    int size = atoi(argv[1]);
    int a[size];
    for (int i = 0; i < size; i++){
        a[i] = 10*i;
    }
    printf(1, "overflow: finish grow stack\n");
    a[0] = a[size-1];  // dummy, used as breakpoint
    exit();
}
