#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include "prog.h"

// Function that executes the command asked by the user
void execute(char **comand, char* cmd) {
    int res, status;
    int bytes_written, bytes_read;
    int fd, fd_par[2];
    pid_t pid;
    struct prog server, par, buffer;

    // Creates pipe to parent process
    res = pipe(fd_par);
    if (res == -1)
        perror("Error creating pipe to parent.");

    // Creates child process
    pid = fork();
    if (pid == 0) {
        struct timeval tv1;

        // Opens pipe to server
        fd = open("pipe_to_server", O_WRONLY);
        if (fd == -1) {
            perror("Error opening pipe_to_server.");
            _exit(1);
        }

        // Closes the reading file descriptor of the pipe to the parent process
        res = close(fd_par[0]); 
        if (res == -1) {
            perror("Error closing reading file descriptor of pipe to parent.");
            _exit(1);
        }

        // Calculates the PID of the current process
        pid_t child_pid;
        child_pid = getpid();
        if (child_pid == -1) {
            perror("Error getting the child PID.");
            _exit(1);
        }

        // Notifies user about the PID of the program that will be run.
        char str[32];
        res = snprintf(str, sizeof(str), "Running PID %d\n", child_pid);
        if (res < 0) {
            perror("Error formatting string.");
            _exit(1);
        }
        bytes_written = write(1, str, res);
        if (bytes_written == -1)
            perror("Error writing to STDOUT.");

        // Stores the PID in the struct that will be sent to the parent process and to the server
        par.pid = child_pid;
        server.pid = child_pid;

        // Stores the name of the program that will be run on the struct that will be sent to the server
        strcpy(server.cmd, cmd);

        // Gets time of day
        res = gettimeofday(&tv1, NULL);      
        if (res == -1) {
            perror("Error getting time of day.");
            _exit(1);
        }
        
        // Stores the value of time in the structs that will be sent to the parent process and to the server
        server.start = tv1;
        par.start = tv1;

        // Sends the struct with the PID and the start time of the program to the parent process
        bytes_written = write(fd_par[1], &par, sizeof(par));
        if (bytes_written == -1)
            perror("Error writing to pipe_to_parent.");

        // Sends the struct with the PID, the name and start time of the program to the server
        bytes_written = write(fd, &server, sizeof(struct prog));
        if (bytes_written == -1)
            perror("Error writing to pipe_to_server.");

        // Closes the pipe to the server
        res = close(fd);
        if (res == -1)
            perror("Error closing pipe_to_server.");

        // Closes the pipe to the parent process
        res = close(fd_par[1]);
        if (res == -1)
            perror("Error closing pipe_to_parent.");
        
        // Executes the program
        res = execvp(comand[0], comand);
        if (res == -1) {
            perror("Error executing execvp.");
            exit(1);
        }
    }
    else if (pid == -1) {
        perror("Erro no fork.");
        _exit(1);
    }
    else {
        // Waits for child process to end
        res = wait(&status);
        if (res == -1) {
            perror("Error executing waitpid.");
            _exit(1);
        }

        if (WIFEXITED(status)) {
            struct timeval end;
            
            // Gets time of day
            res = gettimeofday(&end, NULL);
            if (res == -1) {
                perror("Error getting time of day.");
                _exit(1);
            }

            // Closes the writing file descriptor of the pipe_to_parent
            res = close(fd_par[1]);
            if (res == -1) {
                perror("Error closing writing file descriptor of pipe to parent");
                _exit(1);
            }
            
            // Opens the pipe to the server
            fd = open("pipe_to_server", O_WRONLY);
            if (fd == -1) {
                perror("Error opening pipe_to_server.");
                _exit(1);
            }
            
            // Reads the struct with the PID and the time the program started
            bytes_read = read(fd_par[0], &buffer, sizeof(buffer));
            if (bytes_read == 0) {
                perror("Error reading from pipe to parent.");
                _exit(1);
            }

            // Stores the PID value that was read from the child process and stores it in the struct that will be sent to the server
            server.pid = buffer.pid;
            
            // Calculates the execution time in ms and shows it to the user
            int diff = (end.tv_usec-buffer.start.tv_usec)/1000 + (end.tv_sec-buffer.start.tv_sec)*1000;
            char str[32];
            res = snprintf(str, sizeof(str), "Ended in %d ms\n", diff);
            if (res < 0) {
                perror("Error formatting string.");
                _exit(1);
            }
            bytes_written = write(1, str, strlen(str)); 
            if (bytes_written == 0) {
                perror("Error writing to STDOUT.");
            }

            // Stores the value in ms in the struct that will be sent to the server
            server.ms = diff;
            bytes_written = write(fd, &server, sizeof(struct prog));
            if (bytes_written == 0) {
                perror("Error writing to pipe_to_server.");
                _exit(1);
            }
        }

        // Closes the file descriptor of the pipe to the server
        res = close(fd);
        if (res == -1) {
            perror("Error closing pipe_to_server.");
            _exit(1);
        }

        // Closes the reading file descriptor of the pipe to the parent process
        res = close(fd_par[0]);
        if (res == -1) {
            perror("Error closing reading file descriptor of pipe_to_parent.");
            _exit(1);
        }
    }
}