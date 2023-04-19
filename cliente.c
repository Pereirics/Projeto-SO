#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_TOKENS 1000

void tokenize(char* comando, char** argv) {
    int token_number = 0;    
    char* cmd = strdup(comando);
    char* token = NULL;
    int pid, status, ret;

    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '\n') {
            cmd[i] = '\0';
            break;
        }
    }
    
    token = strsep(&cmd, " ");

    while (token != NULL && token_number < MAX_TOKENS) {
        argv[token_number] = token;
        //printf("%s!", argv[token_number]);
        token = strsep(&cmd, " ");
        token_number++;
    }

    argv[token_number] = NULL;

}

void execute(char** comand) {
    int ret, status, fd;
    char pid_str[32];
    char sec_str[32];
    char usec_str[32]; 
    pid_t pid;
    struct timeval tv;

    pid = fork();
    if (pid == 0) {
        pid_t child_pid;
        child_pid = getpid();

        printf("PID do programa a executar: %d\n", child_pid);
        
        fd = open("pipe", O_WRONLY);
    
        sprintf(pid_str, "%d", child_pid);
        write(fd, pid_str, strlen(pid_str)+1);

        write(fd, comand[2], strlen(comand[2]));
        
        gettimeofday(&tv, NULL);

        sprintf(sec_str, "%ld", tv.tv_sec);
        sprintf(usec_str, "%ld", tv.tv_usec);
        write(fd, sec_str, strlen(sec_str)+1);
        write(fd, usec_str, strlen(usec_str)+1);

        close(fd);
        ret = execvp(comand[2], comand+2);
        if (ret == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid == -1) {
        perror("fork");
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            ret = WEXITSTATUS(status);
        } else {
            ret = -1;
        }
    }
}

int main(int argc, char** argv) {
    int fd, bytes_read;
    char buffer[4096];
    char* store[MAX_TOKENS];

    while ((bytes_read = read(0, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_read] = '\0';
        tokenize(buffer, store);
        if (!strcmp(store[0], "execute") && !strcmp(store[1], "-u"))
            execute(store);
    }
    
    close(fd);

    return 0;
}
