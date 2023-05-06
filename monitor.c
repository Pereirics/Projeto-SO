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
    char args[256][7];
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

void stats_time(int args[], char* folder) {

    int fd, bytes_read;
    char str[64];
    char buffer[100];
    int total = 0;

    for(int i=0; i<256 && args[i] != 0; i++) {
        str[64];
        snprintf(str, sizeof(str), "%s%d.txt", folder, args[i]);
        fd = open(str, O_RDONLY);
        char c;
        int bytes_read=0;

        while(read(fd, &c, sizeof(c)) == 1){
            buffer[bytes_read++] = c;
            if (c == '\n')
                break;
        }

        total += atoi(buffer);
    }

    close(fd);

    fd = open("pipe1", O_WRONLY);

    char res[10];
    snprintf(str, sizeof(str), "%d\n", total);
    write(fd, str, strlen(str)); 

    close(fd);
}

void stats_command(char* cmd, int args[], char* folder) {

    int fd;
    char str[64];
    char buffer[100];
    int total = 0;

    for(int i=0; i<255 && args[i] != 0; i++) {
        str[64];
        snprintf(str, sizeof(str), "%s%d.txt", folder, args[i]);
        fd = open(str, O_RDONLY);
        
        char c;
        int bytes_read=0;

        while(read(fd, &c, sizeof(c)) == 1){
            if (c != '\n') {
                buffer[bytes_read++] = c;
            }
            else {    
                if (!strcmp(buffer, cmd)) 
                    total++;
                strcpy(buffer, "");
                bytes_read = 0;
            }
        }
    }

    close(fd);

    fd = open("pipe1", O_WRONLY);

    char res[10];
    snprintf(res, sizeof(res), "%d\n", total);
    write(fd, res, strlen(res)); 

    close(fd);
}


int main(int argc, char** argv) {

    int res, res1, fd1, fd2, fd_pids, bytes_read, i=0;
    prog buffer;
    prog store[100];

    res = mkfifo("pipe", 0666); 
    res1 = mkfifo("pipe1", 0666);

    fd1 = open("pipe", O_RDONLY);
    fd2 = open("pipe", O_WRONLY); // Mantém o servidor a correr    
    
    while ((bytes_read = read(fd1, &buffer, sizeof(buffer))) > 0) {
        if (!strcmp(buffer.cmd, "status")) {
            status(store, i);
        }
        else if (!strcmp(buffer.cmd, "stats-time")) {    
            int args[256] = {0};
            for (int i = 0; i<256 && strcmp(buffer.args[i], "") != 0; i++) {   
                args[i] = atoi(buffer.args[i]);
            }
            stats_time(args, argv[1]);
        }
        else if (!strcmp(buffer.cmd, "stats-command")) {
            int args[255] = {0};
            for (int i = 0; i<255 && strcmp(buffer.args[i+1], "") != 0; i++) {   
                args[i] = atoi(buffer.args[i+1]);
            }
            stats_command(buffer.args[0], args, argv[1]);
        }
        else {
            int flag = 0, pos;
            for(int j=0; j<=i; j++) { 
                if (store[j].pid == buffer.pid) {
                    char file[64];
                    snprintf(file, sizeof(file), "%s/%d.txt", argv[1], store[j].pid);
                    int fd_pids = open(file, O_CREAT | O_WRONLY, 0666);
                    
                    char str1[128];
                    snprintf(str1, sizeof(str1), "%d\n", buffer.ms);
                    write(fd_pids, str1, strlen(str1));

                    char str2[260];
                    snprintf(str2, sizeof(str2), "%s\n",store[j].cmd);
                    int a = write(fd_pids, str2, strlen(str2));
                    
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