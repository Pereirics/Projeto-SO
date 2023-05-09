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

void stats_time(char args[][7], char* folder) {

    int fd, bytes_read;
    char str[64];
    char buffer[100] = "";
    int total = 0;

    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        str[64];
        snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        fd = open(str, O_RDONLY);
        char c;
        bytes_read=0;

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

void stats_command(char* cmd, char args[][7], char* folder) {

    int fd;
    char c, str[64];
    char buffer[100] = "";
    int bytes_read = 0, total = 0;

    for(int i=0; i<255 && strcmp(args[i], "") != 0; i++) {
        str[64];
        snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        fd = open(str, O_RDONLY);
        
        bytes_read=0;

        while(read(fd, &c, sizeof(c)) == 1){
            if (c != '\n' && c != ' ') {
                buffer[bytes_read++] = c;
            }
            else {    
                buffer[bytes_read] = '\0';
                if (!strcmp(buffer, cmd)) { 
                    total++;
                }
                strcpy(buffer, "");
                bytes_read = 0;
            }
        }
        close(fd);
    }

    fd = open("pipe1", O_WRONLY);

    char res[10];
    snprintf(res, sizeof(res), "%d\n", total);
    write(fd, res, strlen(res)); 

    close(fd);
}

void stats_uniq(char args[][7], char* folder) {

    int fd;
    char c, str[64];
    char buffer[100] = "";
    int total = 0, pos = 0, bytes_read=0, linha, flag_args = 0;
    char store[100][20];

    for (int i=0; i<100; i++) {
        strcpy(store[i], "");
    }

    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        str[64];
        snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        fd = open(str, O_RDONLY);
        
        strcpy(buffer, "");

        linha = 0;

        while(read(fd, &c, sizeof(c)) == 1){
            if (c != '\n' && c != ' ') {
                buffer[bytes_read++] = c;
            }
            else if (c == '\n' && linha < 1) {
                linha++;
                strcpy(buffer, "");
                bytes_read = 0;
            }
            else if (linha >= 1 && !flag_args){    
                buffer[bytes_read] = '\0';
                for (int i=0; i<=pos; i++) {
                    if (pos > 0 && !strcmp(buffer, store[i])) {
                        strcpy(buffer, "");
                        bytes_read = 0;
                        break;
                    }
                    else if (pos == 0 || (i == pos && strcmp(buffer, store[i]) != 0)) {
                        strcpy(store[pos++], buffer); 
                        printf("%s\n", store[pos-1]);
                        strcpy(buffer, "");
                        bytes_read = 0;
                        break;
                    }
                }
                strcpy(buffer, "");
                bytes_read = 0;
            }

            if (c == ' ') 
                flag_args = 1;
            else if (c == '\n')
                flag_args = 0;
        }

        strcpy(buffer, "");
        bytes_read = 0;

        close(fd);
    }

    fd = open("pipe1", O_WRONLY);

    for(int i=0; i<pos; i++) {
        write(fd, store[i], strlen(store[i]));
        write(fd, "\n", 1);
    }

    close(fd);
}

int main(int argc, char** argv) {

    int res_fifo, res1_fifo, fd_read, fd_write, fd_pids, bytes_read, i=0;
    prog buffer, store[100];

    res_fifo = mkfifo("pipe", 0666); 
    res1_fifo = mkfifo("pipe1", 0666);

    fd_read = open("pipe", O_RDONLY);
    fd_write = open("pipe", O_WRONLY); // Mantém o servidor a correr    
    
    while ((bytes_read = read(fd_read, &buffer, sizeof(buffer))) > 0) {
        if (!strcmp(buffer.cmd, "status")) {
            status(store, i);
        }
        else if (!strcmp(buffer.cmd, "stats-time")) {    
            stats_time(buffer.args, argv[1]);
        }
        else if (!strcmp(buffer.cmd, "stats-command")) {
            stats_command(buffer.args[0], buffer.args+1, argv[1]);
        }
        else if (!strcmp(buffer.cmd, "stats-uniq")) {
            stats_uniq(buffer.args, argv[1]);
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

    close(fd_read);
    close(fd_write);

    unlink("pipe");
    unlink("pipe1");

    return 0;
}