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

// Function that executes a pipeline asked by the user
void pipeline(char** store[], int num, char* cmd) {

    int res, fd, bytes_written, pid;
    int pipes[num-1][2];
    int status[num];
    prog server;
    struct timeval start, end;
    
    // Opens pipe to the server
    fd = open("pipe_to_server", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_server.");
        _exit(1);
    }

    // Calculates the PID of the current process (PID of the pipeline)
    server.pid = getpid();
    if (server.pid == -1) {
        perror("Error getting PID.");
        _exit(1);
    }
    
    // Stores the program name in the struct that will be sent to the server
    strcpy(server.cmd, cmd);

    // Shows the user the PID of the running program (pipeline)
    char str[64];
    res = snprintf(str, sizeof(str), "Running PID %d\n", server.pid);
    if (res < 0) {
        perror("Error formatting string.");
        _exit(1);
    }
    bytes_written = write(1, str, strlen(str));
    if (bytes_written == -1) {
        perror("Error writing to STDOUT.");
        _exit(1);
    }

    // Gets the time of day and stores the starting time in the struct that will be sent to the server
    res = gettimeofday(&start, NULL);
    if (res == -1) {
        perror("Error getting time of day.");
        _exit(1);
    }
    server.start = start;

    // Sends the struct with the PID, program name (pipeline) and starting time to the server
    bytes_written = write(fd, &server, sizeof(server));
    if (bytes_written == -1) {
        perror("Error writing to pipe_to_server.");
        _exit(1);
    }

    // Cycle to execute all the programs
    for (int i=0; i<num; i++) {
        // First program to be executed
        if (i==0) {
            // Opens the first pipe
            res = pipe(pipes[i]);
            if (res == -1) {
                perror("Error creating the pipe.");
                _exit(1);
            }
            // Creates child process
            if ((pid = fork())==0) {
                // Closes reading file descriptor of the current pipe
                res = close(pipes[i][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }

                // The file descriptor 1 gets the value of the writing file descriptor of the current pipe
                res = dup2(pipes[i][1], 1);
                if (res == -1) {
                    perror("Erro executing dup.");
                    _exit(1);
                }

                // Closes the writing file descriptor of the current pipe as we will no longer write anything to it
                res = close(pipes[i][1]);
                if (res == -1) {
                    perror("Error closing the writing file descriptor of the pipe.");
                    _exit(1);
                }

                // Executes the first program
                res = execvp(store[i][0], store[i]);
                if (res == -1) {
                    perror("Erro executing execvp.");
                    _exit(1);
                }
            }
            else if (pid == -1){
                perror("Error executing fork.");
                _exit(1);
            }
            else {
                // Closes the writing file descriptor of the current pipe as we will no longer write anything to it
                res = close(pipes[i][1]);
                if (res == -1) {
                    perror("Error closing the writing file descriptor of the pipe.");
                    _exit(1);
                }
            }
        }
        // Last program to be executed
        else if (i==num-1) {
            // Creates child process
            if ((pid = fork()) == 0) {
                // The file descriptor 0 gets the value of the reading file descriptor of the last pipe
                res = dup2(pipes[i-1][0], 0);
                if (res == -1) {
                    perror("Error executing dup.");
                    _exit(1);
                }

                // Closes the reading file descriptor of the last pipe
                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }

                // Executes the last program
                res = execvp(store[i][0], store[i]);
                if (res == -1) {
                    perror("Error executing execvp.");
                    _exit(1);
                }
            }
            else if(pid == -1) {
                perror("Error executing fork.");
                _exit(1);
            }
            else {
                // Closes the reading file descriptor of the last pipe
                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }
            }
        }
        // Middle programs to be executed
        else {
            // Creates the pipe that will connect to the next program
            res = pipe(pipes[i]);
            if (res == -1) {
                perror("Error creating pipe.");
                _exit(1);
            }

            // Creates child process
            if ((pid = fork()) == 0) {
                // Closes reading file descriptor of the current pipe
                close(pipes[i][0]);

                // The file descriptor 0 gets the value of the reading file descriptor of the last pipe
                res = dup2(pipes[i-1][0], 0);
                if (res == -1) {
                    perror("Error executing dup.");
                    _exit(1);
                }

                // Closes the reading file descriptor of the last pipe
                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
                }

                // The file descriptor 1 gets the value of the writing file descriptor of the current pipe
                res = dup2(pipes[i][1], 1);
                if (res == -1)
                    perror("Error executing dup.");
                
                // Closes the writing file descriptor of the current pipe
                close(pipes[i][1]);

                // Executes the program
                res = execvp(store[i][0], store[i]);
                if (res == -1) {
                    perror("Error executing execvp.");
                    _exit(1);
                }
            }
            else if(pid == -1) {
                perror("Error executing fork.");
            }

            // Closes the writing file descriptor of the current pipe
            res = close(pipes[i][1]);
            if (res == -1) {
                perror("Error closing the writing file descriptor of the pipe.");
                _exit(1);
            }
            
            // Closes the reading file descriptor of the last pipe
            res = close(pipes[i-1][0]);
            if (res == -1) {
                perror("Error closing the reading file descriptor of the pipe.");
            }
        }
    }

    // Waits for all programs to end
    for (int i=0; i<num; i++) {
        res = wait(&status[i]);
        if (res == -1) {
            perror("Error executing wait.");
            _exit(1);
        }
    }

    // Gets time of day
    res = gettimeofday(&end, NULL);
    if (res == -1) {
        perror("Error getting time of day.");
        _exit(1);
    }

    // Calculates execution time in ms
    int diff = (end.tv_usec-start.tv_usec)/1000 + (end.tv_sec-start.tv_sec)*1000;
    res = snprintf(str, sizeof(str), "Ended in %d ms\n", diff);
    if (res == -1) {
        perror("Error formatting string.");
        _exit(1);
    }
    
    // Shows the user the execution time in ms
    bytes_written = write(1, str, strlen(str));
    if (bytes_written == -1) {
        perror("Error writing to STDOUT.");
        _exit(1);
    }

    // Stores the execution time in ms in the struct that will be sent to the server
    server.ms = diff;

    // Sends the struct with the execution time to the server (That struct already had the PID, so it was alreay identified)
    bytes_written = write(fd, &server, sizeof(server));
    if (bytes_written == -1) {
        perror("Error writing to pipe_to_server.");
    }
    
    // Closes the pipe to the server
    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_server.");
    }
}
