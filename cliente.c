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

void tokenize(char *comando, char **argv)
{
    int token_number = 0;
    char *cmd = strdup(comando);
    char *token = NULL;
    int pid, status, ret;

    for (int i = 0; cmd[i] != '\0'; i++)
    {
        if (cmd[i] == '\n')
        {
            cmd[i] = '\0';
            break;
        }
    }

    token = strsep(&cmd, " "); // encontra ' ' numa string e torna em '\n';

    while (token != NULL && token_number < MAX_TOKENS)
    {
        argv[token_number] = token;
        // printf("%s!", argv[token_number]);
        token = strsep(&cmd, " "); // strstep function keeps track of the last position in the input string where it found a delimiter character, using a static variable internally.
        token_number++;
    }

    argv[token_number] = NULL;
}

void execute(char **comand)
{
    int ret, status, fd, fd1[2], fd2[2];
    char pid_str[32];
    char sec_str[32];
    char usec_str[32];
    pid_t pid;

    pipe(fd1); // this pipe will be used to send the start time of the command to the parent process.
    pipe(fd2);

    pid = fork();
    if (pid == 0)
    {
        struct timeval tv1;

        fd = open("pipe", O_WRONLY);
        close(fd1[0]); // we close the read end of the pipe (fd1[0]) since the child process only needs to write to it.
        close(fd2[0]);

        pid_t child_pid;
        child_pid = getpid();

        // Indica ao utilizador o PID do programa a executar
        char pid_str[64];
        int len = snprintf(pid_str, sizeof(pid_str), "PID do programa a executar: %d\n", child_pid);
        write(1, pid_str, len);

        // Indica ao servidor e ao processo-pai o PID do programa a executar
        snprintf(pid_str, sizeof(pid_str), "%d\n", child_pid);
        write(fd, pid_str, strlen(pid_str));     // servidor
        write(fd1[1], pid_str, strlen(pid_str)); // pai

        // Indica ao servidor o nome do programa a executar
        write(fd, comand[2], sizeof(comand[2]));
        write(fd, "\n", strlen("\n"));

        // Calcula o timestamp
        gettimeofday(&tv1, NULL);      
        
        // Passa o timestamp ao servidor
        snprintf(sec_str, sizeof(sec_str), "%ld\n", tv1.tv_sec);
        write(fd, sec_str, strlen(sec_str));
        snprintf(usec_str, sizeof(usec_str), "%ld\n", tv1.tv_usec);
        write(fd, usec_str, strlen(usec_str));

        // Passa o timestamp ao processo-pai
        write(fd2[1], &tv1, sizeof(tv1));
       
        close(fd);
        close(fd1[1]);
        close(fd2[1]);

        ret = execvp(comand[2], comand + 2);
        if (ret == -1)
        {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid == -1)
    {
        perror("fork");
    }
    else
    {

        fd = open("pipe", O_WRONLY);
        close(fd1[1]);
        close(fd2[1]);

        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {

            struct timeval start, end;

            // Lê o PID do programa que acabou de executar e passa ao servidor
            char buffer[32];
            int bytes_read = read(fd1[0], buffer, sizeof(buffer));
            buffer[bytes_read] = '\0';
            write(fd, buffer, bytes_read);

            // Lê o tempo inicial, calcula o final e calcula a diferença entre os dois (tempo de execução) 
            gettimeofday(&end, NULL);
            bytes_read = read(fd2[0], &start, sizeof(start));
            long diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
            char str[32];
            snprintf(str, sizeof(str), "Tempo de execução:%ld\n", diff);
            write(1, str, strlen(str)); 

            // Passa o tempo de execução para o servidor em ms
            char diff_str[32];
            snprintf(diff_str, sizeof(diff_str), "%ld", diff);
            write(fd, diff_str, strlen(diff_str));
            write(fd, "\n", strlen(diff_str));
        }
        else
        {
            ret = -1;
        }

        close(fd);
        close(fd1[0]);
    }
}

int main(int argc, char **argv)
{
    int fd, bytes_read;
    char buffer[4096];
    char *store[MAX_TOKENS];

    fd = open("pipe", O_WRONLY);

    while ((bytes_read = read(0, buffer, sizeof(buffer))) > 0)
    {
        buffer[bytes_read] = '\0';
        tokenize(buffer, store);
        if (!strcmp(store[0], "execute") && !strcmp(store[1], "-u"))
            execute(store);
        else if (!strcmp(store[0], "status"))
            write(fd, store[0], sizeof(store[0]));
    }

    close(fd);

    return 0;
}
