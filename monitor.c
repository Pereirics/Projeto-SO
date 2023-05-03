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
    struct timeval start;
    int ms;
} prog;

void status(struct prog store[], int N) {

    struct timeval now;
    gettimeofday(&now, NULL);

    int fd = open("pipe1", O_WRONLY);
   
    for(int i=0; i<N; i++) {
        long diff = (now.tv_usec-store[i].start.tv_usec)/1000 + (now.tv_sec-store[i].start.tv_sec)*1000;
        char str[32];
        int len = snprintf(str, sizeof(str), "%d %s %ld ms\n", store[i].pid, store[i].cmd, diff);
        write(fd, str, len);
    }

    close(fd);

}


int main(int argc, char** argv) {

    int res, res1, fd1, fd2, fd_pids, bytes_read, i=0;
    struct prog buffer;
    struct prog store[100];

    res = mkfifo("pipe", 0666); 
    res1 = mkfifo("pipe1", 0666);

    fd1 = open("pipe", O_RDONLY);
    fd2 = open("pipe", O_WRONLY); // Mantém o servidor a correr    
    
    while ((bytes_read = read(fd1, &buffer, sizeof(buffer))) > 0) {
        if (!strcmp(buffer.cmd, "status")) {
            status(store, i);
        }
        else {
            int flag = 0, pos;
            for(int j=0; j<=i; j++) { 
                if (store[j].pid == buffer.pid) {
                    char file[64];
                    snprintf(file, sizeof(file), "%s/%d.txt", argv[1], store[j].pid);
                    int fd_pids = open(file, O_CREAT | O_WRONLY, 0666);
                    
                    char str1[260];
                    snprintf(str1, sizeof(str1), "%s\n", store[j].cmd);
                    int a = write(fd_pids, str1, strlen(str1));

                    char str2[128];
                    snprintf(str2, sizeof(str2), "Tempo de execução em ms:%d\n", buffer.ms);
                    write(fd_pids, str2, strlen(str2));
                    
                    close(fd_pids);

                    store[j] = store[i-1];
                    i--;
                    flag = 1;
                    break;
            }
        }
        if (!flag) {
            store[i] = buffer;
            i++;
        }
        }
    }   

    close(fd1);
    close(fd2);

    unlink("pipe");

    return 0;
}