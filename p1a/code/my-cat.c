#include <stdlib.h>
#include <stdio.h>

const int LINE_SIZE = 2147483647;

int main(int argc, char *argv[])
{
    if (argc < 2)
        exit(0);
    
    unsigned file_num = 1;
    FILE *fp;
    size_t line_size = LINE_SIZE;
    char *line = (char *)malloc(line_size * sizeof(char));
    
    while (file_num < argc) {
        fp = fopen(argv[file_num], "r");

        if (fp == NULL) {
            printf("my-cat: cannot open file\n");
            exit(1);
        }
        while ((fgets(line, LINE_SIZE, fp) != NULL)) {
            printf("%s", line);
        }
        fclose(fp);
        file_num++;
    }
	
    free(line);
    exit(0);
}
