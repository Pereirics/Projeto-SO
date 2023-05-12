#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "../inc/prog.h"
#include "../inc/stats.h"
#include "../inc/status.h"

#define MAX_TOKENS 1000

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