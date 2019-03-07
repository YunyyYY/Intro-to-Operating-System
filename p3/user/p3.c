#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{

    char a[16];
    char *b = (char *)malloc(sizeof(char)*16);

    char c[16];
    char *d = (char *)malloc(sizeof(char)*16);

    printf(1, "stack: start address of a %p\n", a);
    printf(1, "stack: end address of a %p\n", &a[15]);
    printf(1, "heap: address of b %p\n", b);

    printf(1, "stack: start address of a %p\n", c);
    printf(1, "stack: end address of a %p\n", &c[15]);
    printf(1, "heap: address of b %p\n", d);

    exit();
}
