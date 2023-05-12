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
        char str[269];

        // Formats the string
        res = snprintf(str, sizeof(str), "%d %s %d ms\n", store[i].pid, store[i].cmd, diff);
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
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        // Open the file descriptor of the file with the PID name (Reading)
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

        // Closes the file descriptor of the file with the PID name
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
    res = snprintf(str, sizeof(str), "Total execution time is %d ms\n", total);
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
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        // Opens the file descriptor of the file with the PID name (Reading)
        fd = open(str, O_RDONLY);
        if (fd == -1) {
            perror("Error opening the file with PID name.");
            _exit(1);
        }
        
        // Reads from the file bytes by byte to calculate the number of times the command appears in that file
        pos=0;
        while((bytes_read = read(fd, &c, sizeof(c))) == 1){
            
            // Checks if the char read is different from '\n' (end of line) and stores it in the buffer
            if (c != '\n') {
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

        // Closes the file descriptor of the file with the PID name
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
    res = snprintf(str, sizeof(str), "%s was executed %d times\n", cmd, total);
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
    int pos = 0, pos_store = 0, bytes_read = 0, line;
    char store[100][20];

    // Initializes the array to an empty string in all positions
    for (int i=0; i<100; i++) {
        strcpy(store[i], "");
    }

    // Cycles through the PIDs given by the user and stores the commands in the store array (only once per command)
    for(int i=0; i<256 && strcmp(args[i], "") != 0; i++) {
        
        // Formats the string that gives the path to the file with the PID name
        res = snprintf(str, sizeof(str), "%s%s.txt", folder, args[i]);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }

        // Opens a file descriptor of the file with the PID name (Reading)
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
            
            // If the char read is different from a '\n' (end of line) it stores the char in the buffer
            if (c != '\n') {
                buffer[pos++] = c;
            }
            
            // If the char is a '\n' (end of line) and we are on the first line, it ignores what was read (execution time in ms)
            else if (c == '\n' && line < 1) {
                line++;
                strcpy(buffer, "");
                pos = 0;
            }
            
            // If we are not on the first line, check if the command will be stored
            else if (line >= 1){    

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
        }

        if (bytes_read == -1) {
            perror("Error reading from file with PID name.");
            _exit(1);
        }

        // Sets the buffer to an empty string
        strcpy(buffer, "");
        pos = 0;

        // Closes the file descriptor of the file with the PID name
        res = close(fd);
        if (res == -1) {
            perror("Error closing file with PID name.");
            _exit(1);
        }
    }

    // Opens a pipe to the client (Writing)
    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        _exit(1);
    }

    // Cycles through the array where the uniq commands were stored and writes them to the client, one by one
    for(int i=0; i<pos_store; i++) {
        bytes_written = write(fd, store[i], strlen(store[i]));
        if (bytes_written == -1) {
            perror("Error writting to client.");
            _exit(1);
        }

        // Adds a '\n' so it only shows one by line
        bytes_written = write(fd, "\n", 1);
        if (bytes_written == -1) {
            perror("Error writing to client.");
            _exit(1);
        }
    }

    // Closes the pipe to the client
    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        _exit(1);
    }
}

int main(int argc, char** argv) {

    int fd_read, fd_write, fd_pids;
    int res, bytes_read, bytes_written;
    int flag, pos_store=0;
    prog buffer, store[100];

    // Checks if the number of arguments is correct
    if (argc == 2) {

        // Creates a pipe to read from the client
        if(mkfifo("pipe_to_server", 0666) == -1) {
            perror("Error creating fifo that connects to server."); 
            _exit(1);
        }

        // Creates a pipe to write to the client
        if(mkfifo("pipe_to_client", 0666) == -1) {
            perror("Error creating fifo that connects to client.");
            _exit(1);
        }

        // Opens the pipe to the server (Reading)
        fd_read = open("pipe_to_server", O_RDONLY);
        if (fd_read == -1) {
            perror("Error opening pipe_to_server.");
            _exit(1);
        }

        // This keeps the server running as the file descriptor will block if no reading file descriptors are open
        fd_write = open("pipe_to_server", O_WRONLY); 
        if (fd_write == -1) {
            perror("Error opening pipe_to_server");
            _exit(1);
        }

        // Reads from the client structs prog
        while ((bytes_read = read(fd_read, &buffer, sizeof(buffer))) > 0) {
            // If the command read from the client is "status", executes function associated with that functionality
            if (!strcmp(buffer.cmd, "status")) {
                status(store, pos_store);
            }
            // If the command read from the client is "stats-time", executes function associated with that functionality
            else if (!strcmp(buffer.cmd, "stats-time")) {    
                stats_time(buffer.args, argv[1]);
            }
            // If the command read from the client is "stats-command", executes function associated with that functionality
            else if (!strcmp(buffer.cmd, "stats-command")) {
                stats_command(buffer.args[0], buffer.args+1, argv[1]);
            }
            // If the command read from the client is "stats-uniq", executes function associated with that functionality
            else if (!strcmp(buffer.cmd, "stats-uniq")) {
                stats_uniq(buffer.args, argv[1]);
            }
            // If the command read from the client is "kill", it breaks the cycle so the program monitor will end 
            else if (!strcmp(buffer.cmd, "kill")) {
                break;
            }
            // The struct will only enter here when a new program is being notified to the server, or when a program has finished executing
            else {
                flag = 0;
                // Cycles through the store array to check if the program is new or an existing one
                for(int i=0; i <= pos_store; i++) { 
                    // If it is an existing one, it will create a new file with the PID name and store the running time and the commands used in it
                    if (store[i].pid == buffer.pid) {
                        
                        // Formats the string that will be the path to the file created with the PID as the name
                        char file[64];
                        snprintf(file, sizeof(file), "%s/%d.txt", argv[1], store[i].pid);
                        fd_pids = open(file, O_CREAT | O_WRONLY, 0666);
                        if (fd_pids == -1) 
                            perror("Error opening/creating the file with the PID name.");

                        // Formats the string that will be the running time of the program
                        char str1[128];
                        res = snprintf(str1, sizeof(str1), "%d\n", buffer.ms);
                        if (res < 0) 
                            perror("Error formatting string.");
                        
                        // Writes that same string to the file with the PID name
                        bytes_written = write(fd_pids, str1, strlen(str1));
                        if (bytes_written == -1) 
                            perror("Error writing to the file with the PID name.");

                        // Formats the string that will be the commands of the program
                        char str2[260];
                        res = snprintf(str2, sizeof(str2), "%s\n", store[i].cmd);
                        if (res < 0) 
                            perror("Error formatting string.");

                        // Writes that same string to the file with the PID name
                        bytes_written = write(fd_pids, str2, strlen(str2));
                        if (bytes_written == -1)
                            perror("Error writing to the file with the PID name.");

                        // Closes the file descriptor of the file with the PID name
                        res = close(fd_pids);
                        if (res == -1)
                            perror("Error closing the file with PID name.");

                        // "Removes" that element from the array, as it is no longer a running program
                        store[i] = store[pos_store-1];
                        pos_store--;

                        // Sets the flag to 1 to not store this struct in the array store (that contains the running programs)
                        flag = 1;
                        break;
                    }
                }

                // If it is a new program, stores the struct in the store array
                if (!flag) {
                    store[pos_store] = buffer;
                    pos_store++;
                }
            }
        }   

        if (bytes_read == -1)
            perror("Error reading from client.");

        // Closes the pipe to the server 
        res = close(fd_read);
        if (res == -1)
            perror("Error closing fd_read.");
        res = close(fd_write);

        // Closes the pipe to the client
        if (res == -1)
            perror("Error closing fd_write.");

        // Deletes the fifo that read from the client
        res = unlink("pipe_to_server");
        if (res == -1)
            perror("Error deleting pipe_to_server.");

        // Deletes the fifo that wrote to the client
        res = unlink("pipe_to_client");
        if (res == -1)
            perror("Error deleting pipe_to_client.");

    }
    else 
        perror("Error with the number of arguments.");

    return 0;
}