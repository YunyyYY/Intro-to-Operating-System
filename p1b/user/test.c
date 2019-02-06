#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// test the function int getopenedcount(void);

int
main(int argc, char *argv[])
{
    int count_before = getopenedcount();
    int close_before = getclosedcount();
    int k;
    for (int i = 0; i < 10;i++) {
        k = open("test.c", O_RDONLY);
        if (i < 7)
            close(k);
    }
    int count_after = getopenedcount();
    int close_after = getclosedcount();
    printf(1, "Called open() %d times.\n", count_after - count_before);
    printf(1, "Closed %d files.\n", close_after - close_before);
    exit();
}
