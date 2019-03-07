#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    dummy();
    if (argc < 2) {
        printf(1, "spin currently does nothing!\n");
        exit();
    }

    dummy();
    printf(1, "Let's spin for a while...\n");
    int a = 0;
    for (;a < atoi(argv[1]);a++){
        if (a == 0)
            printf(1, "first round here...\n");
    }
    printf(1, "OMG I survived... \n");
    exit();
}
