#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if(argc != 2)
        exit();
    int i, x;
    for (i = 1; i < atoi(argv[1]); i++) {
        // printf(1, "inside spin\n");
        x = x + i;
    }
    exit();
    return 0;
}