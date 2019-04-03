#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapreduce.h"

void Map(char *file_name) {

    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);

    char *line = NULL;
    size_t size = 0;

    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;  //printf(" %s", line); fflush(stdout);
        while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
            if (*token=='\0')
                break;
            // printf("20| token: %s\n", token);
            MR_Emit(token, "1");
        }
    }  //    printf("count: %d\n", value);
    free(line);
    fclose(fp);
}

void Reduce(char *key, Getter get_next, int partition_number) {
    int count = 0;
    char *value;
    while ((value = get_next(key, partition_number)) != NULL)
        count++;
    printf("%s %d\n", key, count);
}

int main(int argc, char *argv[]) {
    int num = 25;
    argc = 4;
    argv[1] = "input/read130.in";
    argv[2] = "input/read420.in";
    argv[3] = "input/read490.in";
    MR_Run(argc, argv, Map, num, Reduce, num, MR_DefaultHashPartition);
}
