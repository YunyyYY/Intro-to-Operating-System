#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "pstat.h"

int
main(int argc, char *argv[])
{
    struct pstat info;
    printf(1, "start\n");
    getpinfo(&info);
    for (int i = 0; i < NPROC; i++){
        if (info.inuse[i]!=0)
            printf(1, "pid=%d\n", info.pid[i]);
    }
    printf(1, "finish\n");
    exit();
}