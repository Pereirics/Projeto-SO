#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "prog.h"

#define MAX_TOKENS 1000

int tokenize(char* comando, char **store, char* sep) {
    int token_number = 0;
    char *token = NULL;

    token = strsep(&comando, sep); 
    while (token != NULL && token_number < MAX_TOKENS) {
        if (!strcmp("\0", token))
            store[token_number] = NULL;
        else {
            store[token_number] = token;
            token_number++;
        }
        token = strsep(&comando, sep);
        
    }

    store[token_number] = NULL;

    return token_number;
}

void execute(char **comand, char* cmd) {
    int ret, res, bytes_written, bytes_read, status, fd, fd_pai[2];
    pid_t pid;

    struct prog servidor, pai, buffer;

    res = pipe(fd_pai);
    if (res == -1)
        perror("Error creating pipe_to_parent.");

    pid = fork();
    if (pid == 0) {
        struct timeval tv1;

        fd = open("pipe_to_server", O_WRONLY);
        if (fd == -1) {
            perror("Error opening pipe_to_server.");
            _exit(1);
        }

        res = close(fd_pai[0]); 
        if (res == -1) {
            perror("Error closing reading file descriptor of pipe_to_parent.");
            _exit(1);
        }

        pid_t child_pid;
        child_pid = getpid();
        if (child_pid == -1) {
            perror("Error getting the child PID.");
            _exit(1);
        }

        char str[32];
        res = snprintf(str, sizeof(str), "Running PID %d\n", child_pid);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }
        
        bytes_written = write(1, str, res);
        if (bytes_written == -1)
            perror("Error writing to STDOUT.");

        pai.pid = child_pid;
        servidor.pid = child_pid;

        strcpy(servidor.cmd, cmd);

        res = gettimeofday(&tv1, NULL);      
        if (res == -1) {
            perror("Error getting time of day.");
            _exit(1);
        }
        
        servidor.start = tv1;
        pai.start = tv1;

        bytes_written = write(fd_pai[1], &pai, sizeof(pai));
        if (bytes_written == -1)
            perror("Error writing to pipe_to_parent.");

        bytes_written = write(fd, &servidor, sizeof(struct prog));
        if (bytes_written == -1)
            perror("Error writing to pipe_to_server.");
       
        res = close(fd);
        if (res == -1)
            perror("Error closing pipe_to_server.");

        res = close(fd_pai[1]);
        if (res == -1)
            perror("Error closing pipe_to_parent.");

        ret = execvp(comand[0], comand);
        if (ret == -1) {
            perror("Error executing execvp.");
            exit(1);
        }
    }
    else if (pid == -1) {
        perror("Erro no fork.");
        _exit(1);
    }
    else {
        res = waitpid(pid, &status, 0);
        if (res == -1) {
            perror("Error executing waitpid.");
            _exit(1);
        }

        if (WIFEXITED(status)) {
            struct timeval end;
            
            res = gettimeofday(&end, NULL);
            if (res == -1) {
                perror("Error getting time of day.");
                _exit(1);
            }

            res = close(fd_pai[1]);
            if (res == -1) {
                perror("Error closing writing file descriptor of pipe_to_parent");
                _exit(1);
            }

            fd = open("pipe_to_server", O_WRONLY);
            if (fd == -1) {
                perror("Error opening pipe_to_server.");
                _exit(1);
            }
            
            bytes_read = read(fd_pai[0], &buffer, sizeof(buffer));
            if (bytes_read == 0) {
                perror("Error reading from pipe_to_parent.");
                _exit(1);
            }

            servidor.pid = buffer.pid;
                        
            int diff = (end.tv_usec-buffer.start.tv_usec)/1000 + (end.tv_sec-buffer.start.tv_sec)*1000;
            char str[32];
            res = snprintf(str, sizeof(str), "Ended in %d ms\n", diff);
            if (res < 0) {
                perror("Error formatting string.");
                _exit(1);
            }

            bytes_written = write(1, str, strlen(str)); 
            if (bytes_written == 0) {
                perror("Error writing to STDOUT.");
            }

            servidor.ms = diff;
            bytes_written = write(fd, &servidor, sizeof(struct prog));
            if (bytes_written == 0) {
                perror("Error writing to pipe_to_server.");
                _exit(1);
            }
        }

        res = close(fd);
        if (res == -1) {
            perror("Error closing pipe_to_server.");
            _exit(1);
        }

        res = close(fd_pai[0]);
        if (res == -1) {
            perror("Error closing reading file descriptor of pipe_to_parent.");
            _exit(1);
        }
    }
}

void pipeline(char** store[], int num, char* cmd) {

    int res, fd, bytes_written, pid;
    int pipes[num-1][2];
    int status[num];

    fd = open("pipe_to_server", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_server.");
        _exit(1);
    }

    prog p;
    p.pid = getpid();
    if (p.pid == -1) {
        perror("Error getting PID.");
        _exit(1);
    }
    strcpy(p.cmd, cmd);

    char str[64];
    res = snprintf(str, sizeof(str), "Running PID %d\n", p.pid);
    if (res < 0) {
        perror("Error formatting string.");
        _exit(1);
    }

    bytes_written = write(1, str, strlen(str));
    if (bytes_written == -1) {
        perror("Error writing to STDOUT.");
        _exit(1);
    }

    struct timeval start, end;
    res = gettimeofday(&start, NULL);
    if (res == -1) {
        perror("Error getting time of day.");
        _exit(1);
    }

    p.start = start;

    bytes_written = write(fd, &p, sizeof(p));
    if (bytes_written == -1) {
        perror("Error writing to pipe_to_server.");
        _exit(1);
    }

    res = pipe(pipes[0]);
    if (res == -1) {
        perror("Error creating pipe.");
        _exit(1);
    }

    for (int i=0; i<num; i++) {

        if (i==0) {
            res = pipe(pipes[i]);
            if (res == -1) {
                perror("Error creating the pipe.");
                _exit(1);
            }
            if ((pid = fork())==0) {
                res = close(pipes[i][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }

                int res = dup2(pipes[i][1], 1);
                if (res == -1) {
                    perror("Erro executing dup.");
                    _exit(1);
                }

                res = close(pipes[i][1]);
                if (res == -1) {
                    perror("Error closing the writing file descriptor of the pipe.");
                    _exit(1);
                }

                res = execvp(store[i][0], store[i]);
                if (res == -1) {
                    perror("Erro executing execvp.");
                    _exit(1);
                }
            }
            else if (pid == -1){
                perror("Error executing fork.");
                _exit(1);
            }
            else {
                res = close(pipes[i][1]);
                if (res == -1) {
                    perror("Error closing the writing file descriptor of the pipe.");
                    _exit(1);
                }
            }
        }
        else if (i==num-1) {
            if ((pid = fork()) == 0) {
                res = dup2(pipes[i-1][0], 0);
                if (res == -1) {
                    perror("Error executing dup.");
                    _exit(1);
                }

                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }

                res = execvp(store[i][0], store[i]);
                if (res == -1) {
                    perror("Error executing execvp.");
                    _exit(1);
                }
            }
            else if(pid == -1) {
                perror("Error executing fork.");
                _exit(1);
            }
            else
                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }
        }
        else {
            res = pipe(pipes[i]);
            if (res == -1) {
                perror("Error creating pipe.");
                _exit(1);
            }

            if ((pid = fork()) == 0) {
                close(pipes[i][0]);

                int res=dup2(pipes[i-1][0], 0);
                if (res == -1) {
                    perror("Error executing dup.");
                    _exit(1);
                }

                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }

                res = dup2(pipes[i][1], 1);
                if (res == -1)
                    perror("Error executing dup.");
                close(pipes[i][1]);

                res=execvp(store[i][0], store[i]);
                if (res == -1) {
                    perror("Error executing execvp.");
                    _exit(1);
                }
            }
            else if(pid == -1) {
                perror("Error executing fork.");
            }

            res = close(pipes[i][1]);
            if (res == -1) {
                perror("Error closing the writing file descriptor of the pipe.");
                _exit(1);
            }
            
            res = close(pipes[i-1][0]);
            if (res == -1) {
                perror("Error closing the reading file descriptor of the pipe.");
            }
        }
    }

    for (int i=0; i<num; i++) {
        res = wait(&status[i]);
        if (res == -1) {
            perror("Error executing wait.");
            _exit(1);
        }
    }

    res = gettimeofday(&end, NULL);
    if (res == -1) {
        perror("Error getting time of day.");
        _exit(1);
    }

    int diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
    res = snprintf(str, sizeof(str), "Ended in %d ms\n", diff);
    if (res == -1) {
        perror("Error formatting string.");
        _exit(1);
    }
    
    bytes_written = write(1, str, strlen(str));
    if (bytes_written == -1) {
        perror("Error writing to STDOUT.");
        _exit(1);
    }

    p.ms = diff;
    bytes_written = write(fd, &p, sizeof(p));
    if (bytes_written == -1) {
        perror("Error writing to pipe_to_server.");
    }

    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_server.");
    }
}


int main(int argc, char **argv) {
    int fd_write, fd_read, bytes_read, status;
    char *store[MAX_TOKENS];
    char cmd[MAX_TOKENS];
    char buffer[100];
    struct prog st;

    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        if (argc == 4) {
            strcpy(cmd, argv[3]);
            tokenize(argv[3], store, " ");
            execute(store, cmd);
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    else if (!strcmp(argv[1], "status")) {
        strcpy(st.cmd, argv[1]);
        write(fd_write, &st, sizeof(st));
        fd_read = open("pipe_to_client", O_RDONLY);
        while ((bytes_read = read(fd_read, &buffer, sizeof(buffer))) > 0) {
                write(1, buffer, bytes_read);
            }
        close(fd_read);
    }
    else if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-p")) {
        int num = tokenize(argv[3], store, "|");
        char** new_store[num];
        char dest[1024] = "";
        for (int i=0; i<num; i++) {
            new_store[i] = malloc(MAX_TOKENS * sizeof(char*));
            tokenize(store[i], new_store[i], " ");
            strcat(dest, new_store[i][0]);
            if (i != num-1) 
                strcat(dest, " | ");
        }
        pipeline(new_store, num, dest);
    }
    else if (!strcmp(argv[1], "stats-time")) {

        fd_write = open("pipe_to_server", O_WRONLY);
        prog p;
        strcpy(p.cmd, argv[1]);
        
        int i, j;
        for(i=0; i<256; i++)
            strcpy(p.args[i], "");

        for (i = 2, j = 0; i < argc; i++, j++) {
            strcpy(p.args[j], argv[i]);
            p.args[j][strlen(argv[i])] = '\0'; 
        }

        write(fd_write, &p, sizeof(struct prog));

        close(fd_write);

        fd_read = open("pipe_to_client", O_RDONLY);
        char buffer[10];
        int bytes_read = read(fd_read, &buffer, sizeof(buffer));
        write(1, buffer, bytes_read);
        close(fd_read);
    }
    else if (!strcmp(argv[1], "stats-command")) {

        fd_write = open("pipe_to_server", O_WRONLY);
        prog p;

        strcpy(p.cmd, argv[1]);

        int i, j;
        for(i=0; i<256; i++)
            strcpy(p.args[i], "");

        strcpy(p.args[0], argv[2]);

        for (i = 3, j = 1; i < argc; i++, j++) {
            strcpy(p.args[j], argv[i]);
            p.args[j][strlen(argv[i])] = '\0'; 
        }

        write(fd_write, &p, sizeof(struct prog));

        close(fd_write);

        fd_read = open("pipe_to_client", O_RDONLY);
        char buffer[10];
        int bytes_read = read(fd_read, &buffer, sizeof(buffer));
        write(1, buffer, bytes_read);
        close(fd_read);
    }
    else if (!strcmp(argv[1], "stats-uniq")) {
        
        fd_write = open("pipe_to_server", O_WRONLY);
        prog p;

        strcpy(p.cmd, argv[1]);

        for(int i=0; i<256; i++)
            strcpy(p.args[i], "");

        strcpy(p.args[0], argv[2]);

        for (int i = 3, j = 1; i < argc; i++, j++) {
            strcpy(p.args[j], argv[i]);
            p.args[j][strlen(argv[i])] = '\0'; 
        }

        write(fd_write, &p, sizeof(struct prog));

        close(fd_write);

        fd_read = open("pipe_to_client", O_RDONLY);

        while((bytes_read = read(fd_read, &buffer, sizeof(buffer))) > 0) {
            write(1, buffer, bytes_read);
        }

        close(fd_read);

    }

    return 0;
}
