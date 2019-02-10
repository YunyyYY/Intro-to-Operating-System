#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

const int LINE_SIZE = 1024;
const int PROG_LEN = 64;
const int ARG_SIZE = 16;
const int PATH_SIZE = 8;
const char error_message[30] = "An error has occurred\n";

struct cmd_log {
    char cmd[LINE_SIZE];
    struct cmd_log * next;
};

int main(int argc, char* argv[]) {

// ----------------- I/O part -----------------

    int stdin_flag = 1;  // flag of input

    FILE *io = stdin;
    if (argc == 1)
        printf("wish> ");
    else if (argc == 2) {
        stdin_flag = 0;
        io = fopen(argv[1], "r");
        if (io == NULL) {  // if file fail to open
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

// ----------------- run part -----------------

    // read command variables, [NEED FREE]
    size_t line_size = LINE_SIZE;
    char * line = (char *)malloc(line_size * sizeof(char));

    // parse command line variables
    char * token, * child_argv[ARG_SIZE];
    int max_arg = 0, child_argc;  // argc counts real number of arguments
    char prog[PROG_LEN];  // [NEED FREE]

    // history logs
    int num_log = 0;
    struct cmd_log * init_log = NULL, * cur_log = NULL;  // use to store

    while (getline(&line, &line_size, io) != -1) {
        struct cmd_log * tmp_log = (struct cmd_log *)malloc(sizeof(struct cmd_log));
        strcpy(tmp_log->cmd, line);
        tmp_log->next = NULL;
        if (num_log == 0)
            init_log = tmp_log;
        else
            cur_log->next = tmp_log;
        cur_log = tmp_log;
        // built-int command [exit]
        if (strcmp(line, "exit\n") == 0) // printf("wish: bye!\n");
            break; // break for exit

        // get and parse first word command
        token = strtok(line, " \n");
        child_argc = 0;

        // first check if built-in commands
        if (strcmp(token, "exit") == 0) // cannot be [exit] anymore
            write(STDERR_FILENO, error_message, strlen(error_message));

        while(token != NULL) {


            child_argv[child_argc++] = strdup(token);
            token = strtok(NULL, " \n");
        }
        child_argv[child_argc] = NULL;
        if (max_arg < child_argc)
            max_arg = child_argc;

        if (strcmp(child_argv[0], "cd") == 0) {  // ------- [cd] -------
            if (child_argc != 2)
                write(STDERR_FILENO, error_message, strlen(error_message));
            int ret = chdir(child_argv[1]);  // if dir doesn't exist, return -1
            if (ret !=0)
                write(STDERR_FILENO, error_message, strlen(error_message));
            // check_dir();
        } else if (strcmp(child_argv[0], "history") == 0) {  // --- [history] ---
            if (child_argc > 2)
                write(STDERR_FILENO, error_message, strlen(error_message));
            else {
                int count = 0, n_max = 0, n_flag = 0;
                if (child_argc == 2) {
                    n_flag = 1;
                    char *err = NULL;
                    double n = strtod(child_argv[1], &err);
                    if (*err != '\0')
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    n_max = (int)n;
                    if (n != n_max)
                        n_max = n_max + 1;
                }
                struct cmd_log * tmp = init_log;
                while (tmp != NULL) {
                    if (n_flag && (++count > n_max))
                        break;
                    printf("%s", tmp->cmd);
                    tmp = tmp->next;
                }
            }
        } else if (strcmp(child_argv[0], "path") == 0) { // ------- [path] ---------
            // [path]
        } else {  // ---------- not built-in command ----------
            //use fork() to call it
        }

        num_log = num_log + 1;
        if (stdin_flag)
            printf("wish> ");
    }


    // in case of terminal interruption or end-of-file

    for (int i = 0; i != max_arg; i++)
        free(child_argv[i]);
    struct cmd_log * vim;
    while (init_log != NULL) {
        vim = init_log;
        free(vim);
        init_log = init_log->next;
    }
    free(line);
    exit(0);
}