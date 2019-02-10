#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>


const int LINE_SIZE = 1024;
const int ARG_LEN = 128;
const int PROG_LEN = 64;
const int PATH_SIZE = 32;
const int ARG_SIZE = 16;
const char error_message[30] = "An error has occurred\n";

struct cmd_log {
    char *his;
    struct cmd_log * next;
    struct cmd_log * prev;
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

    // history logs
    int num_log = 0;
    struct cmd_log * init_log = NULL, * cur_log = NULL;  // use to store

    // path
    char * path[PATH_SIZE];
    for (int i = 0; i < PATH_SIZE; i++)
        path[i] = (char *)malloc(ARG_LEN * sizeof(char));
    int num_path = 1;
    strcpy(path[0], "/bin/");

    while (getline(&line, &line_size, io) != -1) {

        struct cmd_log * tmp_log = (struct cmd_log *)malloc(sizeof(struct cmd_log));
        tmp_log->his = strdup(line);
        tmp_log->next = NULL;
        tmp_log->prev = cur_log;
        if (num_log == 0)
            init_log = tmp_log;
        else
            cur_log->next = tmp_log;
        cur_log = tmp_log;

        // built-int command [exit]
        if (strcmp(line, "exit\n") == 0 | strcmp(line, "exit") == 0) // printf("wish: bye!\n");
            break; // break for exit

        // get and parse first word command
        token = strtok(line, " \t\n");
        child_argc = 0;

        // first check if built-in commands
        if (strcmp(token, "exit") == 0) // cannot be [exit] anymore
            write(STDERR_FILENO, error_message, strlen(error_message));
        else {  // parsing the entire command
            while(token != NULL) {
                child_argv[child_argc++] = strdup(token);
                token = strtok(NULL, " \t\n");
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
                    int n_max = num_log;
                    char *err;
                    if (child_argc == 2) {
                        double n = strtod(child_argv[1], &err);
                        if ((*err)!='\0')
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        n_max = (int)n;
                        if (n == n_max)
                            n_max = n_max - 1;
                    }

                    struct cmd_log * tmp = cur_log;
                    for (int i = n_max; i != 0; i--) {
                        tmp = tmp->prev;
                    }
                    for (int i = -1; i != n_max; i++) {
                        if (tmp==NULL) {printf("%d\n", i);break;}
                        printf("%s", tmp->his);
                        tmp = tmp->next;
                    }
                }
            } else if (strcmp(child_argv[0], "path") == 0) { // ------- [path] ---------
                num_path = child_argc - 1;
                for (int i = 0; i < num_path; i++) {
                    strcpy(path[i], child_argv[i+1]);
                    strcat(path[i], "/");  //  printf("%s\n", path[i]);
                }
            } else {  // ---------- not built-in command ----------
                int wstatus, ret = -1;
                char prog[PROG_LEN];  // [NEED FREE]
                for (int i = 0; i < num_path; i++) {
                    strcpy(prog, "\0");
                    strcat(prog, path[i]);
                    strcat(prog, child_argv[0]);
                    ret = access(prog, X_OK);  // printf("access %s: %d\n", prog, ret);
                    if (ret == 0) {
                        pid_t rc = fork();
                        if (rc == 0) {  // child program

                            execv(prog, child_argv);
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        } else {
                            waitpid(rc, &wstatus, 0);  // printf("child finish in main!\n");
                            break;
                        }
                    }
                }
                if (ret != 0)
                    write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }

        num_log = num_log + 1;
        if (stdin_flag)
            printf("wish> ");
    }


    // in case of terminal interruption or end-of-file

    for (int i = 0; i != max_arg; i++)
        free(child_argv[i]);
    for (int i = 0; i < PATH_SIZE; i++)
        free(path[i]);
    struct cmd_log * vim;
    while (init_log != NULL) {
        vim = init_log;
        free(vim);
        init_log = init_log->next;
    }
    free(line);
    exit(0);
}