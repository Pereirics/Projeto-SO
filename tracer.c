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
    int ret, status, fd, fd_pai[2];
    pid_t pid;

    struct prog servidor, pai, buffer;

    pipe(fd_pai);
    pid = fork();
    if (pid == 0)
    {
        struct timeval tv1;

        fd = open("pipe_to_server", O_WRONLY);

        close(fd_pai[0]); 

        pid_t child_pid;
        child_pid = getpid();

        char str[32];
        int len = snprintf(str, sizeof(str), "Running PID %d\n", child_pid);
        write(1, str, len);

        pai.pid = child_pid;
        servidor.pid = child_pid;

        strcpy(servidor.cmd, cmd);

        gettimeofday(&tv1, NULL);      
        
        servidor.start = tv1;
        pai.start = tv1;

        write(fd_pai[1], &pai, sizeof(pai));
        write(fd, &servidor, sizeof(struct prog));
       
        close(fd);
        close(fd_pai[1]);

        ret = execvp(comand[0], comand);
        if (ret == -1)
        {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            struct timeval end;
            
            gettimeofday(&end, NULL);

            close(fd_pai[1]);

            fd = open("pipe_to_server", O_WRONLY);
            
            int bytes_read = read(fd_pai[0], &buffer, sizeof(buffer));
            servidor.pid = buffer.pid;
                        
            int diff = (end.tv_usec-buffer.start.tv_usec)/1000 + (end.tv_sec-buffer.start.tv_sec)*1000;
            char str[32];
            snprintf(str, sizeof(str), "Ended in %d ms\n", diff);
            write(1, str, strlen(str)); 

            servidor.ms = diff;
            write(fd, &servidor, sizeof(struct prog));
        }
        close(fd);
        close(fd_pai[0]);
    }
}

void pipeline(char** store[], int num, char* cmd) {

    int pipes[num-1][2];
    int status[num];

    int fd = open("pipe_to_server", O_WRONLY);
    prog p;
    p.pid = getpid();
    strcpy(p.cmd, cmd);

    char str[64];
    snprintf(str, sizeof(str), "Running PID %d\n", p.pid);
    write(1, str, strlen(str));

    struct timeval start, end;
    gettimeofday(&start, NULL);
    p.start = start;

    write(fd, &p, sizeof(p));

    pipe(pipes[0]);

    if (fork()==0) {
        close(pipes[0][0]);
        dup2(pipes[0][1], 1);
        close(pipes[0][1]);

        execvp(store[0][0], store[0]);
    }
    
    for (int i=1; i<num-1; i++) {
        close(pipes[i-1][1]);
        pipe(pipes[i]);
        
        if (fork() == 0) {
            close(pipes[i][0]);

            dup2(pipes[i-1][0], 0);
            close(pipes[i-1][0]);

            dup2(pipes[i][1], 1);
            close(pipes[i][1]);

            execvp(store[i][0], store[i]);
       }

       close(pipes[i][1]);
       close(pipes[i-1][0]);
    }

    if (fork() == 0) {
        close(pipes[num-2][1]);

        dup2(pipes[num-2][0], 0);
        close(pipes[num-2][0]);

        execvp(store[num-1][0], store[num-1]);
    }

    close(pipes[num-2][0]);

    for (int i=0; i<num; i++) {
        wait(&status[i]);
    }

    gettimeofday(&end, NULL);

    int diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
    snprintf(str, sizeof(str), "Ended in %d ms\n", diff);
    write(1, str, strlen(str));

    p.ms = diff;
    write(fd, &p, sizeof(p));

    close(fd);
}


int main(int argc, char **argv) {
    int fd_write, fd_read, bytes_read, status;
    char *store[MAX_TOKENS];
    char cmd[MAX_TOKENS];
    char buffer[100];
    struct prog st;

    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        strcpy(cmd, argv[3]);
        tokenize(argv[3], store, " ");
        execute(store, cmd);
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
