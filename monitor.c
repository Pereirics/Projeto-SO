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

// Calculates the current running time to every program, formats the string showing the results and sends them to the client
void status(struct prog store[], int N) {

    int res, fd;
    int diff, bytes_written;
    struct timeval now;
    
    // Gets time of day
    res = gettimeofday(&now, NULL);
    if (res == -1) {
        perror("Error getting time of day.");
        _exit(1);
    }

    // Opens pipe to the client (Writing)
    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }
    
    // Cycles through the array that contains all running programs and calculates the running time until now
    for(int i=0; i<N; i++) {
        diff = (now.tv_usec-store[i].start.tv_usec)/1000 + (now.tv_sec-store[i].start.tv_sec)*1000;
        char str[32];

        // Formats the string
        res = snprintf(str, sizeof(str), "%d %s %ld ms\n", store[i].pid, store[i].cmd, diff);
        if (res < 0) {
            perror("Error formating string.");
            _exit(1);
        }
        
        // Sends the string to the client
        bytes_written = write(fd, str, res);
        if (bytes_written == -1) {
            perror("Error writing to client.");
            _exit(1);
        }

    }

    // Close the pipe to the client
    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }

}

// Calculates the total time in the PIDs given by the user
void stats_time(char args[][7], char* folder) {

    int fd, res, pos;
    int bytes_read, bytes_written;
    int total = 0;
    char str[64];
    char buffer[100] = "";
    char c;
    
    // Cycles through the array that contains the PIDs given by the user and sums the time to the variable total
    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        
        // Formats string that gives the path to the PID file
        str[64];
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        // Open the file of the PID (Reading)
        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening file with PID name.");
            _exit(1);
        }
        
        //  Reads from the file byte by byte until it encounters a '\n' (end of line)
        pos = 0;
        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            buffer[pos++] = c;
            if (c == '\n')
                break;
        }
        if (bytes_read == -1) {
            perror("Error reading from file with PID name.");
            _exit(1);
        }

        // Sums the ms of the file to the total amount
        total += atoi(buffer);

        // Closes the pipe to the file with the PID name
        res = close(fd);
        if (res == -1) {
            perror("Error closing the file with PID name.");
            _exit(1);
        }
    }

    // Opens the pipe to the client (Writing)
    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }

    // Formats the string that will be sent to the server
    str[10];
    res = snprintf(str, sizeof(str), "%d\n", total);
    if (res < 0) {
        perror("Error formatting string.");
        _exit(1);
    }
    
    // Sends the string to the server
    bytes_written = write(fd, str, strlen(str)); 
    if (bytes_written == -1) {
        perror("Error writinf to client.");
        _exit(1);
    }

    // Closes the pipe to the client
    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }
}

// Calculates the number of times a program given by the user was executed in the PIDs given by the user
void stats_command(char* cmd, char args[][7], char* folder) {

    int fd, res, pos;
    char c, str[64];
    char buffer[100] = "";
    int bytes_written, bytes_read = 0, total = 0;

    // Cycles through the PIDs given by the user and calculates the number of times the command was executed in that PID
    for(int i=0; i<255 && strcmp(args[i], "") != 0; i++) {
        
        // Formats the string that gives the path to the file with the PID name
        str[64];
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        // Opens the pipe to the file with the PID name (Reading)
        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening the file with PID name.");
            _exit(1);
        }
        
        // Reads from the file bytes by byte to calculate the number of times the command appears in that file
        pos=0;
        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            
            // Checks if the char read is different from '\n' (end of line) and from a ' ' (beggining of the command arguments) and stores it in the buffer
            if (c != '\n' && c != ' ') {
                buffer[pos++] = c;
            }
            else {    
                // Terminates the string to prevent leaks
                buffer[pos] = '\0';

                // If current string is the same as the command, increases the value by 1 and resets the buffer to empty again for further reading
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

        // Closes the pipe to the file with the PID name
        res = close(fd);
        if (res == -1) {
            perror("Error closing file with PID name.");
            _exit(1);
        }
    }

    // Opens the pipe to the client
    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }

    // Formats the string with the total number of times the command was executed in those PIDs
    str[10];
    res = snprintf(str, sizeof(str), "%d\n", total);
    if (res < 0) {
        perror("Error formatting string.");
        _exit(1);
    }

    // Sends the string to the client
    bytes_written = write(fd, str, strlen(str)); 
    if (bytes_written == -1) {
        perror("Error writing to client.");
        _exit(1);
    }

    // Closes the pipe to the client
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
    int total = 0, pos = 0, pos_store = 0, bytes_read=0, line, flag_args = 0;
    char store[100][20];

    // Initializes the array to an empty string in all positions
    for (int i=0; i<100; i++) {
        strcpy(store[i], "");
    }

    // Cycles through the PIDs given by the user and stores the commands in the store array (only once per command)
    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        
        // Formats the string that gives the path to the file with the PID name
        str[64];
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        // Opens the pipe to the file with the PID name
        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening file with PID name.");
            _exit(1);
        }

        // Sets the buffer to an empty string
        strcpy(buffer, "");

        // Reads from the file byte by byte, storing the new commands in the array store
        line = 0;
        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            
            // If the char read is different from a '\n' (end of line) and from a ' ' (beggining of the arguments) it stores the char in the buffer
            if (c != '\n' && c != ' ') {
                buffer[pos++] = c;
            }
            
            // If the char is a '\n' (end of line) and we are on the first line, it ignores what was read (execution time in ms)
            else if (c == '\n' && line < 1) {
                line++;
                strcpy(buffer, "");
                pos = 0;
            }
            
            // If we are not on the first line and are not reading arguments of a command, check if the command will be stored
            else if (line >= 1 && !flag_args){    

                // Terminates the string to prevent leaks
                buffer[pos] = '\0';

                // Cycles through the commands already stores to see if the command in the buffer is new or not
                for (int i=0; i<=pos_store; i++) {
                    
                    // If the store array is not empty and the buffer is the same as a command in the store array it ignores the buffer
                    if (pos_store > 0 && !strcmp(buffer, store[i])) {
                        strcpy(buffer, "");
                        pos = 0;
                        break;
                    }
                    // If the store array is empty or if we have checked all positions of the store array and the buffer command is different from them all then it stores it in the array
                    else if (pos_store == 0 || (i == pos_store && strcmp(buffer, store[i]) != 0)) {
                        strcpy(store[pos_store++], buffer); 
                        strcpy(buffer, "");
                        pos = 0;
                        break;
                    }
                }

                // Sets the buffer to an empty string
                strcpy(buffer, "");
                pos = 0;
            }

            // If the read char
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

        // Keeps the server running
        fd_write = open("pipe_to_server", O_WRONLY); 
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