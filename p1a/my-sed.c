#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const int LINE_SIZE = 1;

void swap_word(char *line, char *old, char *new){
    unsigned len_old = strlen(old);
    char *start = strstr(line, old);
    if (start == NULL) {
        printf("%s", line);
    }
    else {
        while(line != start){
        printf("%c", *(line++));
        }
        printf("%s", new);
        start += len_old;
        printf("%s", start);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("my-sed: find_term replace_term [file ...]\n");
        exit(1);
    }

    size_t line_size = LINE_SIZE;
    char *line = (char *)malloc(line_size * sizeof(char));

    if (line == NULL) {
        printf("Unable to allocate memory for getline()");
        free(line);
        exit(1);
    }

    if (argc == 3) {  // read from standard input
        while (getline(&line, &line_size, stdin) != -1) {
            swap_word(line, argv[1], argv[2]); // find: argv[1]; replace: argv[2];
        }
    }
    else {  // argc > 3, read from file
        unsigned file_num = 3;
        FILE *fp;
        while (file_num < argc) {
            fp = fopen(argv[file_num], "r");

            if (fp == NULL) {
                printf("my-sed: cannot open file\n");
                free(line);
                exit(1);
            }
			
            while (getline(&line,&line_size,fp) != -1) {
                swap_word(line, argv[1], argv[2]); // find: argv[1]; replace: argv[2];

            }
            fclose(fp);
            file_num++;
        }
    }
    free(line);
    exit(0);
}
