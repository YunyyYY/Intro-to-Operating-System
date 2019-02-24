#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const int LINE_SIZE = 1024;

int main(int argc, char *argv[])
{
    char *prev, *next;
    size_t line_size = LINE_SIZE;
    prev = (char *)malloc(line_size * sizeof(char));
    next = (char *)malloc(line_size * sizeof(char));

    if (argc == 1) {  // read from stdin
        while (getline(&next, &line_size, stdin) != -1) {
            if (strcmp(prev, next) != 0) {
                printf("%s", next);
            }
            strcpy(prev, next);
        }
    }

    else {  // read from file
        unsigned file_num = 1;
        FILE *fp;
        while (file_num < argc) {
            fp = fopen(argv[file_num], "r");
            
            if (fp == NULL) {
                printf("my-uniq: cannot open file\n");
                free(prev);
                free(next);
                exit(1);
            }

            *prev = '\0';
            while (getline(&next, &line_size, fp) != -1) {
                if (strcmp(prev, next) != 0) {
                    printf("%s", next);
                }
                strcpy(prev, next);
            }
            fclose(fp);
            file_num++;
        }
    }
    free(prev);
    free(next);
    exit(0);
}