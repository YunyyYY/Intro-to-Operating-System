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

char * add_space(char * line){
    size_t len = strlen(line);
    char temp[len+3];
    int flag = 0, it = 0, il = 0;
    while (il < len) {
        if ((line[il] == '|') | (line[il] == '>')) {
            if (flag == 1)
                return NULL;
            temp[it++]  = ' ';
            if (line[il] == '|')
                temp[it++] = '|';
            else
                temp[it++] = '>';
            temp[it++]  = ' ';
            il ++;
            flag = 1;
        } else {
            temp[it] = line[il];
            it++;
            il++;
        }
    }
    temp[it]='\0';
    free(line);
    line = strdup (temp);
    return line;
}

int main(int argc, char* argv[]) {

// ----------------- I/O part -----------------

    int stdin_flag = 1;  // flag of input

    FILE * io = stdin;
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

    // redirection and piping
    int sfd2, sfd3, red_flag, pip_flag;
    FILE * redio;

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
/*

        // built-int command [exit]
        if (strcmp(line, "exit\n") == 0 | strcmp(line, "exit") == 0) // printf("wish: bye!\n");
            break; // break for exit
*/
        line = add_space(line);
        if (line == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            num_log = num_log + 1;
            if (stdin_flag)
                printf("wish> ");
            continue;
        }  // printf("the line is %s\n", line);
        red_flag = pip_flag = 0;
        redio = NULL;

        // get and parse first word command
        token = strtok(line, " \t\n");
        child_argc = 0;

        if (token == NULL) {
            num_log = num_log + 1;
            if (stdin_flag)
                printf("wish> ");
            continue;
        }

        // first check if built-in commands
        if (strcmp(token, "exit") == 0) {  // only one word [exit]
            if (strtok(NULL, " \t\n") != NULL) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                num_log = num_log + 1;
                if (stdin_flag)
                    printf("wish> ");
                continue;
            }
            break;
        }

        while(token != NULL) {
            if (strcmp(token,">") == 0) {
                red_flag = 1;
                break;
            } else if (strcmp(token,"|") == 0) {
                pip_flag = 1;
                break;
            }
            child_argv[child_argc++] = strdup(token);
            token = strtok(NULL, " \t\n");
        }

        child_argv[child_argc] = NULL;
        if (max_arg < child_argc)
            max_arg = child_argc;

        if (red_flag) {  // redirection
            token = strtok(NULL, " \t\n");
            redio = fopen(token, "w+");
            if ((strtok(NULL, " \t\n") != NULL) | (redio == NULL)) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                num_log = num_log + 1;
                if (stdin_flag)
                    printf("wish> ");
                continue;
            }
        }

        if (pip_flag) {
            // do the pipe stuff
        }

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
                    if ((n < 0) | ((*err)!='\0')) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        num_log = num_log + 1;
                        if (stdin_flag)
                            printf("wish> ");
                        continue;
                    }
                    n_max = (int)n;
                    if (n == n_max)
                        n_max = n_max - 1;
                }

                struct cmd_log * tmp = cur_log;
                for (int i = n_max; i != 0; i--) {
                    tmp = tmp->prev;
                }
                for (int i = -1; i != n_max; i++) {
                    if (tmp==NULL)
                        break;
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
                        if (red_flag) {
                            dup2(fileno(redio), STDOUT_FILENO);
                            dup2(fileno(redio), STDERR_FILENO);
                            fclose(redio);
                        }
                        execv(prog, child_argv);
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    } else {
                        fclose(redio);  // becasue it is opened in the parent process
                        waitpid(rc, &wstatus, 0);  // printf("child finish in main!\n");
                        break;
                    }
                }
            }
            if (ret != 0)
                write(STDERR_FILENO, error_message, strlen(error_message));
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
