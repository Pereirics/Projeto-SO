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

typedef struct prog 
{
    int pid;
    char cmd[256];
    char* args[256];
    struct timeval start;
    int ms;
} prog;


int tokenize(char* comando, char **store, char* sep) {
    int token_number = 0;
    char *token = NULL;

    token = strsep(&comando, sep); // encontra ' ' numa string e torna em '\0';
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
    int ret, status, fd, fd1[2], fd2[2];
    char pid_str[32];
    char sec_str[32];
    char usec_str[32];
    pid_t pid;

    struct prog novo;

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
        char pid_str1[8];
        snprintf(pid_str1, sizeof(pid_str1), "%d ", child_pid);
        novo.pid = child_pid;
        write(fd1[1], pid_str1, strlen(pid_str1)); // pai

        // Indica ao servidor o nome do programa a executar
        strcpy(novo.cmd, cmd);

        // Calcula o timestamp
        gettimeofday(&tv1, NULL);      
        
        // Passa o timestamp ao servidor
        novo.start = tv1;
        
        // Passa o timestamp ao processo-pai
        write(fd2[1], &tv1, sizeof(tv1));

        write(fd, &novo, sizeof(novo));
       
        close(fd);
        close(fd1[1]);
        close(fd2[1]);

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
        if (WIFEXITED(status))
        {

            struct timeval start, end;
            
            gettimeofday(&end, NULL);

            fd = open("pipe", O_WRONLY);
            close(fd1[1]);
            close(fd2[1]);

            // Lê o PID do programa que acabou de executar e passa ao servidor
            char buffer[8];
            int bytes_read = read(fd1[0], buffer, sizeof(buffer));
            novo.pid = atoi(buffer);
            
            // Lê o tempo inicial, calcula o final e calcula a diferença entre os dois (tempo de execução) 
            bytes_read = read(fd2[0], &start, sizeof(start));
            
            long diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
            char str[32];
            snprintf(str, sizeof(str), "Tempo de execução:%ldms\n", diff);
            write(1, str, strlen(str)); 

            // Passa o tempo de execução para o servidor em ms
            novo.ms = diff;
            write(fd, &novo, sizeof(novo));
        }

        close(fd);
        close(fd1[0]);
        close(fd2[0]);
    }
}

void pipeline(char** store[], int num, char* cmd) {

    int pipes[num-1][2];
    int status[num];

    int fd = open("pipe", O_WRONLY);
    prog p;
    int pid;
    p.pid = getpid();
    strcpy(p.cmd, cmd);

    char str[64];
    snprintf(str, sizeof(str), "PID da pipeline a executar: %d\n", p.pid);
    write(1, str, strlen(str));

    struct timeval start, end;
    gettimeofday(&start, NULL);
    p.start = start;

    write(fd, &p, sizeof(p));
    
    for (int i=0; i<num; i++) {

        if (i==0) {
            pipe(pipes[i]);

            if ((pid = fork())==0) {
                close(pipes[i][0]);
                int res=dup2(pipes[i][1], 1);
                if (res == -1)
                    perror("Erro no dup\n");
                close(pipes[i][1]);

                res = execvp(store[i][0], store[i]);
                _exit(res);
            }
            else if (pid == -1){
                perror("Erro no fork\n");
            }
            else {
                close(pipes[i][1]);
            }
        }
        else if (i==num-1) {
            if ((pid=fork())== 0) {
                int res=dup2(pipes[i-1][0], 0);
                if (res == -1)
                    perror("Erro no dup\n");
                close(pipes[i-1][0]);

                res = execvp(store[i][0], store[i]);
                _exit(res);
            }
            else if(pid == -1) {
                perror("Erro no fork\n");
            }
            else
                close(pipes[i-1][0]);
        }
        else {
            pipe(pipes[i]);
        
            if ((pid=fork()) == 0) {
                close(pipes[i][0]);

                int res=dup2(pipes[i-1][0], 0);
                if (res == -1)
                    perror("Erro no dup\n");
                close(pipes[i-1][0]);

                res=dup2(pipes[i][1], 1);
                if (res == -1)
                    perror("Erro no dup\n");
                close(pipes[i][1]);

                res=execvp(store[i][0], store[i]);
                _exit(res);
            }

            close(pipes[i][1]);
            close(pipes[i-1][0]);
        }
    }

    for (int i=0; i<num; i++) {
        wait(&status[i]);
    }

    gettimeofday(&end, NULL);

    int diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
    snprintf(str, sizeof(str), "Tempo de execução: %dms\n", diff);
    write(1, str, strlen(str));

    p.ms = diff;
    write(fd, &p, sizeof(p));

    close(fd);
}



int main(int argc, char **argv) {
    int fd1, fd2, bytes_read, status;
    char *store[MAX_TOKENS];
    char cmd[MAX_TOKENS];
    char buffer[100];
    struct prog st;

    fd1 = open("pipe", O_WRONLY);    

    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        strcpy(cmd, argv[3]);
        tokenize(argv[3], store, " ");
        execute(store, cmd);
    }
    else if (!strcmp(argv[1], "status")) {
        strcpy(st.cmd, argv[1]);
        write(fd1, &st, sizeof(st));
        fd2 = open("pipe1", O_RDONLY);
        while ((bytes_read = read(fd2, &buffer, sizeof(buffer))) > 0) {
                write(1, buffer, bytes_read);
            }
        close(fd2);
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

        int fd = open("pipe", O_WRONLY);
        prog p;
        strcpy(p.cmd, argv[1]);

        for (int i = 2, j = 0; i < argc; i++, j++) {
            p.args[j] = malloc(strlen(argv[i]) + 1);
            strcpy(p.args[j], argv[i]);
            //p.args[j][strlen(argv[i])] = '\0'; // null-terminate the string
        }


        int a=write(fd, &p, sizeof(prog));
        printf("BYTES WRITTEN: %d\n", a);
        close(fd);

    }
    
    close(fd1);

    return 0;
}
