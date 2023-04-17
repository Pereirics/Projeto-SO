#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_TOKENS 1000

void tokenize(char* comando, char** argv) {
    int token_number = 0;    
    char* cmd = strdup(comando);
    char* token = NULL;
    int pid, status, ret;
    
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
    int ret, status;
    pid_t pid;

    pid = fork();
    if (pid == 0) {
        ret = execvp(comand[0], comand);
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
    char buffer[1024];
    char* store[MAX_TOKENS];
    
    fd = open("pipe", O_WRONLY);

    while ((bytes_read = read(0, buffer, sizeof(buffer))) > 0) {
        tokenize(buffer, store);
        //execute(store);
    }
    
    close(fd);

    return 0;
}
