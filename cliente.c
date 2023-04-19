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
    
    token = strsep(&cmd, " "); //encontra ' ' numa string e torna em '\n';

    while (token != NULL && token_number < MAX_TOKENS) {
        argv[token_number] = token;
        //printf("%s!", argv[token_number]);
        token = strsep(&cmd, " "); //strstep function keeps track of the last position in the input string where it found a delimiter character, using a static variable internally.
        token_number++;
    }

    argv[token_number] = NULL;

}

void execute(char** comand) {
    int ret, status, fd, fd1[2];
    char pid_str[32];
    char sec_str[32];
    char usec_str[32]; 
    pid_t pid;

    pipe(fd1); // this pipe will be used to send the start time of the command to the parent process.

    pid = fork();
    if (pid == 0) {
        struct timeval tv1;

        fd = open("pipe", O_WRONLY);
        close(fd1[0]); // we close the read end of the pipe (fd1[0]) since the child process only needs to write to it.

        pid_t child_pid;
        child_pid = getpid();
        
        // Indica ao utilizador o PID do programa a executar
        char pid_str[64]; 
        int len = snprintf(pid_str, sizeof(pid_str), "PID do programa a executar: %d\n", child_pid);
        write(1, pid_str, len);
        
        // Indica ao servidor e ao processo-pai o PID do programa a executar
        snprintf(pid_str, sizeof(pid_str), "%d\n", child_pid);
        write(fd, pid_str, strlen(pid_str)); //servidor
        write(fd1[1], pid_str, strlen(pid_str)); //pai

        // Indica ao servidor o nome do programa a executar
        write(fd, comand[2], sizeof(comand[2]));
        
        // Calcula o timestamp e passa o mesmo ao processo-pai e ao servidor
        gettimeofday(&tv1, NULL);      
        write(fd1[1], &tv1, sizeof(tv1));
        write(fd, &tv1, sizeof(tv1));

        close(fd);
        close(fd1[1]);

        ret = execvp(comand[2], comand+2);
        if (ret == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid == -1) {
        perror("fork");
    } else {

        fd = open("pipe", O_WRONLY);
        close(fd1[1]);

        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            
            struct timeval start, end;
            
            // Lê o PID do programa que acabou de executar e passa ao servidor
            char buffer[64];
            int bytes_read = read(fd1[0], buffer, sizeof(buffer));
            write(fd, buffer, sizeof(bytes_read));
            
            // Lê o tempo inicial, calcula o final e calcula a diferença entre os dois (tempo de execução)
            gettimeofday(&end, NULL);
            read(fd1[0], &start, sizeof(start));
            long diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
            
            // Passa o tempo de execução para o servidor em ms
            char diff_str[32]; 
            snprintf(diff_str, sizeof(diff_str), "%ld", diff);
            write(fd, diff_str, strlen(diff_str));

        } else {
            ret = -1;
        }

        close(fd);
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
