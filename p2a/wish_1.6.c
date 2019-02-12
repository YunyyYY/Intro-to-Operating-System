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
    if (argc == 2) {
        stdin_flag = 0;
        io = fopen(argv[1], "r");
        if (io == NULL) {  // if file fail to open
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    } else if (argc > 2){
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

// ----------------- run part -----------------

    // read command variables, [NEED FREE]
    size_t line_size = LINE_SIZE;
    char * line = (char *)malloc(line_size * sizeof(char));

    // parse command line variables
    char * token, * child_argv[ARG_SIZE], * sec_argv[ARG_SIZE];
    int max_arg = 0, max_sec = 0, child_argc, sec_argc;  // argc counts real number of arguments

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
    int red_flag, pip_flag;
    FILE * redio;

    while (1) {  printf("%d\n", num_log+1);
        if (stdin_flag) {  // if with file as input, no need fflush
            printf("wish> ");  // fflush is only for out stream
            fflush(stdout);
        }

        free(line);
        line = (char *)malloc(line_size * sizeof(char));
        if (getline(&line, &line_size, io) == -1)
            break;

        struct cmd_log * tmp_log = (struct cmd_log *)malloc(sizeof(struct cmd_log));
        tmp_log->his = (char *)malloc(line_size * sizeof(char));
        strcpy(tmp_log->his, line);
        tmp_log->next = NULL;
        tmp_log->prev = cur_log;
        if (num_log == 0)
            init_log = tmp_log;
        else
            cur_log->next = tmp_log;
        cur_log = tmp_log;
        num_log = num_log + 1;

        line = add_space(line);
        if (line == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            continue;
        }  // printf("the line is %s\n", line);
        red_flag = pip_flag = 0;
        redio = NULL;

        // get and parse first word command
        token = strtok(line, " \n\t");
        child_argc = 0;

        while(token != NULL) {
            if (strcmp(token,">") == 0) {
                red_flag = 1;
                break;
            } else if (strcmp(token,"|") == 0) {
                pip_flag = 1;
                break;
            } // printf("$%d: %d %s\n", num_log, child_argc, token);
            child_argv[child_argc++] = strdup(token);
            token = strtok(NULL, " \t\n");
        }

        if (child_argc == 0)
            continue;
        else if (strcmp(child_argv[0], "exit") == 0) {
            if (child_argc > 1) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            break;
        }

        child_argv[child_argc] = NULL;
        if (max_arg < child_argc)
            max_arg = child_argc;

        if (red_flag) {  // redirection
            token = strtok(NULL, " \t\n");
            redio = fopen(token, "w+");
            if ((strtok(NULL, " \t\n") != NULL) | (redio == NULL)) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
        }

        if (pip_flag) {
            sec_argc = 0;
            token = strtok(NULL, " \t\n");  // drop the '|' and parse next one, if it has
            while(token != NULL) {
                sec_argv[sec_argc++] = strdup(token);
                token = strtok(NULL, " \t\n");
            }
            sec_argv[sec_argc] = NULL;
            if (sec_argc > max_sec)
                max_sec = sec_argc;
        }

        if (strcmp(child_argv[0], "cd") == 0) {  // ------- [cd] -------
            if (child_argc != 2) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            int ret = chdir(child_argv[1]);  // if dir doesn't exist, return -1
            if (ret !=0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            // check_dir();
        } else if (strcmp(child_argv[0], "history") == 0) {  // --- [history] ---
            if (child_argc > 2)
                write(STDERR_FILENO, error_message, strlen(error_message));
            else {
                int n_max = num_log - 1;
                char *err;
                if (child_argc == 2) {
                    double n = strtod(child_argv[1], &err);
                    if ((n < 0) | ((*err)!='\0')) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
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
            int wstatus, ret = -1, p2 = -1;
            char prog[PROG_LEN];  // [NEED FREE]
            for (int i = 0; i < num_path; i++) {
                strcpy(prog, "\0");
                strcat(prog, path[i]);
                strcat(prog, child_argv[0]);
                ret = access(prog, X_OK);  // printf("access %s: %d\n", prog, ret);
                if (ret == 0) {
                    char prog2[PROG_LEN];  // may not use at all;
                    pid_t pid2, rc;
                    int pipefd[2];

                    if (pip_flag) {
                        p2 = 0;
                        for (int j = 0; j < num_path; j++) {
                            strcpy(prog2, "\0");
                            strcat(prog2, path[i]);
                            strcat(prog2, child_argv[0]);
                            ret = access(prog2, X_OK);
                            if (ret == 0) {
                                p2 = 1;
                                break;
                            }
                        }
                        if (p2==0) {  // second program not exists
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            break;
                        }
                        if (pipe(pipefd) == -1) {  // create piping and check
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(EXIT_FAILURE);
                        }
                    }

                    rc = fork();

                    if (rc == 0) {  // child program
                        if (pip_flag) {   printf("first child program\n");
                            close(pipefd[0]);          // Close unused read end
                            execv(prog, child_argv);
                        }
                        else {
                            if (red_flag) {
                                dup2(fileno(redio), STDOUT_FILENO);
                                dup2(fileno(redio), STDERR_FILENO);
                                fclose(redio);
                            }
                            execv(prog, child_argv);
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                    } else {  // parent
                        if (pip_flag)
                            close(pipefd[0]);
                        waitpid(rc, &wstatus, 0);
                        if (p2==1) {
                            pid2 = fork();
                            if (pid2 == 0) {  printf("second child program\n");
                                close(pipefd[1]);  // pipefd[1] refers to the write end of the pip
                                execv(prog2, sec_argv);
                            } else {
                                if (pip_flag)
                                    close(pipefd[1]);
                                printf("first child finish in main!\n");
                                waitpid(pid2, &wstatus, 0); printf("second child finish in main\n");
                            }
                        }
                        if (redio != NULL)
                            fclose(redio);  // becasue it is opened in the parent process
                        break;
                    }
                }
            }
            if (ret != 0 | p2 == 0)
                write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }


    // in case of terminal interruption or end-of-file

    for (int i = 0; i != max_arg; i++)
        free(child_argv[i]);
    for (int i = 0; i != max_sec; i++)
        free(sec_argv[i]);
    for (int i = 0; i < PATH_SIZE; i++)
        free(path[i]);

    struct cmd_log * vim;
    while (init_log != NULL) {
        vim = init_log; // printf("$%d: %s\n", i++, vim->his);
        free(vim);
        init_log = init_log->next;
    }
    free(line);
    exit(0);
}