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

    int res, bytes_written;
    struct timeval now;
    
    res = gettimeofday(&now, NULL);
    if (res == -1) {
        perror("Error getting time of day.");
        _exit(1);
    }

    int fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }
   
    for(int i=0; i<N; i++) {
        long diff = (now.tv_usec-store[i].start.tv_usec)/1000 + (now.tv_sec-store[i].start.tv_sec)*1000;
        char str[32];

        res = snprintf(str, sizeof(str), "%d %s %ld ms\n", store[i].pid, store[i].cmd, diff);
        if (res < 0) {
            perror("Error formating string.");
            _exit(1);
        }
        
        bytes_written = write(fd, str, res);
        if (bytes_written == -1) {
            perror("Error writing to client.");
            _exit(1);
        }

    }

    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }

}

void stats_time(char args[][7], char* folder) {

    int fd, bytes_read, bytes_written, res, pos;
    char str[64];
    char buffer[100] = "";
    int total = 0;

    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        str[64];
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening file with PID name.");
            _exit(1);
        }
        
        char c;
        pos=0;

        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            buffer[pos++] = c;
            if (c == '\n')
                break;
        }

        if (bytes_read == -1) {
            perror("Error reading from file with PID name.");
            _exit(1);
        }

        total += atoi(buffer);

        res = close(fd);
        if (res == -1) {
            perror("Error closing the file with PID name.");
            _exit(1);
        }
    }

    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }

    str[10];
    res = snprintf(str, sizeof(str), "%d\n", total);
    if (res < 0) {
        perror("Error formatting string.");
        _exit(1);
    }
    
    bytes_written = write(fd, str, strlen(str)); 
    if (bytes_written == -1) {
        perror("Error writinf to client.");
        _exit(1);
    }

    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }
}


void stats_command(char* cmd, char args[][7], char* folder) {

    int fd, res, pos;
    char c, str[64];
    char buffer[100] = "";
    int bytes_written, bytes_read = 0, total = 0;

    for(int i=0; i<255 && strcmp(args[i], "") != 0; i++) {
        str[64];
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening the file with PID name.");
            _exit(1);
        }
        
        pos=0;

        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            if (c != '\n' && c != ' ') {
                buffer[pos++] = c;
            }
            else {    
                buffer[pos] = '\0';
                if (!strcmp(buffer, cmd)) { 
                    total++;
                }
                strcpy(buffer, "");
                pos = 0;
            }
        }

        if (bytes_read == -1) {
            perror("Error reading from file with PID name.");
            _exit(1);
        }

        res = close(fd);
        if (res == -1) {
            perror("Error closing file with pid name.");
            _exit(1);
        }
    }

    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }

    str[10];
    res = snprintf(str, sizeof(str), "%d\n", total);
    if (res < 0) {
        perror("Error formatting string.");
        _exit(1);
    }

    bytes_written = write(fd, str, strlen(str)); 
    if (bytes_written == -1) {
        perror("Error writing to client.");
        _exit(1);
    }

    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }
}

void stats_uniq(char args[][7], char* folder) {

    int fd, res, bytes_written;
    char c, str[64];
    char buffer[100] = "";
    int total = 0, pos=0, pos_store = 0, bytes_read=0, linha, flag_args = 0;
    char store[100][20];

    for (int i=0; i<100; i++) {
        strcpy(store[i], "");
    }

    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        str[64];
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening file with PID name.");
            _exit(1);
        }

        strcpy(buffer, "");

        linha = 0;

        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            if (c != '\n' && c != ' ') {
                buffer[pos++] = c;
            }
            else if (c == '\n' && linha < 1) {
                linha++;
                strcpy(buffer, "");
                pos = 0;
            }
            else if (linha >= 1 && !flag_args){    
                buffer[pos] = '\0';
                for (int i=0; i<=pos_store; i++) {
                    if (pos_store > 0 && !strcmp(buffer, store[i])) {
                        strcpy(buffer, "");
                        pos = 0;
                        break;
                    }
                    else if (pos_store == 0 || (i == pos_store && strcmp(buffer, store[i]) != 0)) {
                        strcpy(store[pos_store++], buffer); 
                        strcpy(buffer, "");
                        pos = 0;
                        break;
                    }
                }
                strcpy(buffer, "");
                pos = 0;
            }

            if (c == ' ') 
                flag_args = 1;
            else if (c == '\n')
                flag_args = 0;
        }

        if (bytes_read == -1) {
            perror("Error reading from file with PID name.");
            _exit(1);
        }

        strcpy(buffer, "");
        pos = 0;

        res = close(fd);
        if (res == -1) {
            perror("Error closing file with PID name.");
            _exit(1);
        }
    }

    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }


    for(int i=0; i<pos_store; i++) {
        bytes_written = write(fd, store[i], strlen(store[i]));
        if (bytes_written == -1) {
            perror("Error writting to client.");
            _exit(1);
        }
        
        bytes_written = write(fd, "\n", 1);
        if (bytes_written == -1) {
            perror("Error writing to client.");
            _exit(1);
        }
    }

    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }
}

int main(int argc, char** argv) {

    int res, fd_read, fd_write, fd_pids, bytes_read, bytes_written, i=0;
    prog buffer, store[100];

    if (argc == 2) {

        if(mkfifo("pipe_to_server", 0666) == -1) {
            perror("Error creating fifo that connects to server."); 
            _exit(1);
        }

        if(mkfifo("pipe_to_client", 0666) == -1) {
            perror("Error creating fifo that connects to client.");
            _exit(1);
        }

        fd_read = open("pipe_to_server", O_RDONLY);
        if (fd_read == -1) {
            perror("Error opening pipe_to_server.");
            _exit(1);
        }

        fd_write = open("pipe_to_server", O_WRONLY); // Mantém o servidor a correr    
        if (fd_write == -1) {
            perror("Error opening pipe_to_server");
            _exit(1);
        }

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
            else if (!strcmp(buffer.cmd, "kill")) {
                break;
            }
            else {
                int flag = 0, pos;
                for(int j=0; j<=i; j++) { 
                    if (store[j].pid == buffer.pid) {
                        char file[64];
                        snprintf(file, sizeof(file), "%s/%d.txt", argv[1], store[j].pid);
                        int fd_pids = open(file, O_CREAT | O_WRONLY, 0666);
                        if (fd_pids == -1) 
                            perror("Error opening/creating the file with the PID name.");

                        char str1[128];
                        res = snprintf(str1, sizeof(str1), "%d\n", buffer.ms);
                        if (res < 0) 
                            perror("Error formatting string.");
                        
                        bytes_written = write(fd_pids, str1, strlen(str1));
                        if (bytes_written == -1) 
                            perror("Error writing to the file with the PID name.");

                        char str2[260];
                        snprintf(str2, sizeof(str2), "%s\n", store[j].cmd);
                        bytes_written = write(fd_pids, str2, strlen(str2));
                        if (bytes_written == -1)
                            perror("Error writing to the file with the PID name.");

                        res = close(fd_pids);
                        if (res == -1)
                            perror("Error closing the file with PID name.");

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

        if (bytes_read == -1)
            perror("Error reading from client.");

        res = close(fd_read);
        if (res == -1)
            perror("Error closing fd_read.");
        res = close(fd_write);
        if (res == -1)
            perror("Error closing fd_write.");

        res = unlink("pipe_to_server");
        if (res == -1)
            perror("Error deleting pipe_to_server.");
        res = unlink("pipe_to_client");
        if (res == -1)
            perror("Error deleting pipe_to_client.");

    }
    else 
        perror("Error with the number of arguments.");

    return 0;
}