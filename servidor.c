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

    for(int i=0; i<N; i++) {
        long diff = (now.tv_usec-store[i].start.tv_usec)/1000 + (now.tv_sec-store[i].start.tv_sec)*1000;
        printf("%d %s %ld ms\n", store[i].pid, store[i].cmd, diff);
    }

}



int main(int argc, char** argv) {

    int res, fd1, fd2, fd_ficheiro, bytes_read, i=0;
    struct prog buffer;
    struct prog store[100];

    res = mkfifo("pipe", 0666); // int mkfifo (const char *pathname, mode_t mode); Creates a custom FIFO file with the name filename "pipe". A file that behaves like a pipe is referred to as a FIFO or a named pipe.

    fd1 = open("pipe", O_RDONLY);
    fd2 = open("pipe", O_WRONLY); // Mantém o servidor a correr

    while ((bytes_read = read(fd1, &buffer, sizeof(buffer))) > 0) {
        if (!strcmp(buffer.cmd, "status"))
            status(store, i);
        else {
        int flag = 0, pos;
        for(int j=0; j<=i; j++) { 
            if (store[j].pid == buffer.pid) {
                store[j].ms = buffer.ms;
                flag = 1;
                pos = j;
            }
        }
        /*
        if (!flag) {
            store[i] = buffer;
            printf("PID: %d\n", buffer.pid);
            printf("CMD: %s\n", buffer.cmd);
            printf("Start: %ld %ld\n", buffer.start.tv_sec, buffer.start.tv_usec);
            printf("ms: %d\n", buffer.ms);
        }
        else {
            printf("PID: %d\n", store[pos].pid);
            printf("CMD: %s\n", store[pos].cmd);
            printf("Start: %ld %ld\n", store[pos].start.tv_sec, store[pos].start.tv_usec);
            printf("ms: %d\n", store[pos].ms);
        }
        */
        
        i++;
        }
    }   

    close(fd1);
    close(fd2);
    unlink("pipe");

    return 0;
}