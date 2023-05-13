#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "../inc/prog.h"
#include "../inc/execute.h"
#include "../inc/pipeline.h"
#include "../inc/tokenize.h"

#define MAX_TOKENS 1000

int main(int argc, char **argv) {
    int fd_write, fd_read, bytes_read, bytes_written, res;
    char* store[MAX_TOKENS];
    char cmd[MAX_TOKENS];
    char buffer[100];
    struct prog st;

    // Executes option "execute -u" that executes a program given by the user
    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        if (argc == 4) {

            // Tokenizes the command with the delimiter ' ' and stores it in the string array store
            tokenize(argv[3], store, " ");

            // Stores the name of the program to be sent to the server
            strcpy(cmd, store[0]);

            // Executes the program requested 
            execute(store, cmd);
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    // Executes the option "status" that shows the user the current active programs and its run time until that moment
    else if (!strcmp(argv[1], "status")) {
        if (argc == 2) {
            // Opens a pipe to the server (Writing)
            fd_write = open("pipe_to_server", O_WRONLY);

            // Copies the "status" to the command field of the struct that will be sent to the server
            strcpy(st.cmd, argv[1]);

            // Sends the struct sinalizing the option to be executed to the server
            bytes_written = write(fd_write, &st, sizeof(st));
            if (bytes_written == -1) {
                perror("Error writing to pipe_to_server");
                _exit(1);
            }

            // Opens a pipe to the client (Reading)
            fd_read = open("pipe_to_client", O_RDONLY);
            if (fd_read == -1) {
                perror("Error opening pipe_to_client.");
                _exit(1);
            }

            // Reads from the pipe while there is values to be read
            while ((bytes_read = read(fd_read, &buffer, sizeof(buffer))) > 0) {
                    // Shows the user the value that was read from the server (PID, name of program and execution time until now)
                    bytes_written = write(1, buffer, bytes_read);
                    if (bytes_written == -1) {
                        perror("Error writing to STDOUT.");
                        _exit(1);
                    }
            }

            if (bytes_read == -1) {
                perror("Error reading from pipe_to_client.");
                _exit(1);
            }

            // Closes the pipe to the client
            res = close(fd_read);
            if (res == -1) {
                perror("Error closing pipe_to_client.");
                _exit(1);
            }

            // Closes the pipe to the server
            res = close(fd_write);
            if (res == -1) {
                perror("Error closing pipe_to_server.");
                _exit(1);
            }
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    // Executes option "execute -p" that executes a pipeline given by the user
    else if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-p")) {
        if (argc == 4) {
            char dest[1024] = "";

            // Tokenizes the command with the delimiter "|"
            int num = tokenize(argv[3], store, "|");
            
            char** new_store[num];
            
            // Stores the individual strings in a new array with size of the number os strings resulting from the first tokenize
            for (int i=0; i<num; i++) {
                // Allocates space for the new strings
                new_store[i] = malloc(MAX_TOKENS * sizeof(char*));

                // Tokenizes the string with the delimiter " "
                tokenize(store[i], new_store[i], " ");
                
                // Stores the name of the program (without the arguments) in a string to then be sent to the server 
                strcat(dest, new_store[i][0]);
                
                // Adds a " | " in between the strings unless its the last one
                if (i != num-1) 
                    strcat(dest, " | ");
            }
            
            // Executes the pipeline
            pipeline(new_store, num, dest);
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    // Executes the option stats-time that shows the user the sum of the execution time of the PIDs given by the user
    else if (!strcmp(argv[1], "stats-time")) {
        if (argc > 2) {
            prog server;

            // Opens the pipe to the server (Writing)
            fd_write = open("pipe_to_server", O_WRONLY);
            if (fd_write == -1) {
                perror("Error opening pipe_to_server.");
                _exit(1);
            }

            // Copies the "stats-time" to the command field of the struct that will be sent to the server
            strcpy(server.cmd, argv[1]);

            // Initializes all the positions of the array with an empty string
            for(int i=0; i<256; i++)
                strcpy(server.args[i], "");

            // Goes through argv (ignoring the tracer and stats-time)
            for (int i = 2, j = 0; i < argc; i++, j++) {
                // Stores the PID in the args field of the struct that will be sent to the server
                strcpy(server.args[j], argv[i]);

                // Adds a '\0' to prevent leaks
                server.args[j][strlen(argv[i])] = '\0'; 
            }

            // Sends the struct with the option stats-time, and the PIDs to the server
            bytes_written = write(fd_write, &server, sizeof(struct prog));
            if (bytes_written == -1) {
                perror("Error writing to pipe_to_server.");
                _exit(1);
            }

            // Closes the pipe to the server
            res = close(fd_write);
            if (res == -1) {
                perror("Error closing pipe_to_server.");
                _exit(1);
            }

            // Opens the pipe to the client (Reading)
            fd_read = open("pipe_to_client", O_RDONLY);
            if (fd_read == -1) {
                perror("Error opening pipe_to_client.");
                _exit(1);
            }

            // Reads the contents from the pipe
            char buffer[32];
            bytes_read = read(fd_read, &buffer, sizeof(buffer));
            if (bytes_read == -1) {
                perror("Error reading from pipe_to_client.");
                _exit(1);
            }

            // Shows the user the total ms used by those PIDs
            bytes_written = write(1, buffer, bytes_read);
            if (bytes_written == -1) {
                perror("Error writing to STDOUT.");
                _exit(1);
            }

            // Closes the pipe to the client
            res = close(fd_read);
            if (res == -1) {
                perror("Error closing pipe_to_client.");
                _exit(1);
            }
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    // Executes option stats-command that shows the user the number of times that command was executed in the PIDs given
    else if (!strcmp(argv[1], "stats-command")) {
        if (argc > 3) {
            prog server;

            // Opens pipe to the server (Writing)
            fd_write = open("pipe_to_server", O_WRONLY);

            // Stores the option "stats-command" int the command field of the struct that will be sent to the server
            strcpy(server.cmd, argv[1]);

            // Initializes the args field of the struct as an empty string in all positions
            for(int i=0; i<256; i++)
                strcpy(server.args[i], "");

            // Sets the first position of the args field as the command that will be searched in the PIDs
            strcpy(server.args[0], argv[2]);

            // Goes through the args field and stores the PIDs given by the user (ignoring the tracer, stats-command and the command)
            for (int i = 3, j = 1; i < argc; i++, j++) {
                // Stores the PID in the args field
                strcpy(server.args[j], argv[i]);

                // Adds a '\0' to prevent leaks
                server.args[j][strlen(argv[i])] = '\0'; 
            }

            // Sends the struct with the program to be searched and the PIDs of the files that will be searched
            bytes_written = write(fd_write, &server, sizeof(struct prog));
            if (bytes_written == -1) {
                perror("Error writing to pipe_to_server");
            }

            // Closes the pipe to the server
            res = close(fd_write);
            if (res == -1) {
                perror("Error closing pipe_to_server.");
            }

            // Opens the pipe to the client (Reading)
            fd_read = open("pipe_to_client", O_RDONLY);
            if (fd_read == -1) {
                perror("Error opening pipe_to_client.");
                _exit(1);
            }

            // Reads the contents from the buffer
            char buffer[64];
            bytes_read = read(fd_read, &buffer, sizeof(buffer));
            if (bytes_read == -1) {
                perror("Error reading from pipe_to_client.");
                _exit(1);
            }

            // Shows the user the number o times the command was executed in those PIDs
            bytes_written = write(1, buffer, bytes_read);
            if (bytes_written == -1) {
                perror("Error writing to STDOUT.");
                _exit(1);
            }

            // Closes the pipe to the client
            res = close(fd_read);
            if (res == -1) {
                perror("Error closing pipe_to_client.");
                _exit(1);
            }
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    // Executes the option stats-uniq that shows the user the names of the programs that were executed in the PIDs given (only shows one of each command)
    else if (!strcmp(argv[1], "stats-uniq")) {
        if (argc > 2) {
            prog server;

            // Opens pipe to the server (Writing)
            fd_write = open("pipe_to_server", O_WRONLY);
            if (fd_write == -1) {
                perror("Error opening pipe_to_server.");
                _exit(1);
            }

            // Copies the "stats-uniq" to the command field of the struct that will be sent to the server
            strcpy(server.cmd, argv[1]);

            // Initializes the args field with an empty string on each position
            for(int i=0; i<256; i++)
                strcpy(server.args[i], "");

            // Stores the PIDs given in the args field
            for (int i = 2, j = 0; i < argc; i++, j++) {
                // Stores the PID in the args field
                strcpy(server.args[j], argv[i]);

                // Adds a '\0' to prevent leaks
                server.args[j][strlen(argv[i])] = '\0'; 
            }

            // Sends the struct with the command "stats-uniq" and the PIDs to be searched to the server
            bytes_written = write(fd_write, &server, sizeof(struct prog));
            if (bytes_written == -1) {
                perror("Error writing to pipe_to_server.");
                _exit(1);
            }

            // Closes the pipe to the server
            res = close(fd_write);
            if (res == -1) {
                perror("Error closing pipe_to_server.");
                _exit(1);
            }

            // Opens the pipe to the client (Reading)
            fd_read = open("pipe_to_client", O_RDONLY);
            if (fd_read == -1) {
                perror("Error opening pipe_to_client.");
                _exit(1);
            }

            // Reads the contens sent by the server
            while((bytes_read = read(fd_read, &buffer, sizeof(buffer))) > 0) {
                // Shows the user the command sent by the server, one by line
                bytes_written = write(1, buffer, bytes_read);
                if (bytes_written == -1) {
                    perror("Error writing to STDOUT.");
                    _exit(1);
                }
            }

            if (bytes_read == -1) {
                perror("Error reading from pipe_to_client.");
                _exit(1);
            }
            
            // Closes the pipe to the client
            res = close(fd_read);
            if (res == -1) {
                perror("Error closing pipe_to_client.");
                _exit(1);
            }
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    // Executes option kill that ends the server
    else if (!strcmp(argv[1], "kill")) {
        if (argc == 2) {
            prog server;

            // Copies the option "kill" to the command field of the struct that will be sent to the server
            strcpy(server.cmd, argv[1]);

            // Opens pipe to the server (Writing)
            fd_write = open("pipe_to_server", O_WRONLY);
            if (fd_write == -1) {
                perror("Error opening pipe_to_server.");
                _exit(1);
            }

            // Sends the struct with the command "kill" to the server
            bytes_written = write(fd_write, &server, sizeof(struct prog));
            if (bytes_written == -1) {
                perror("Error writing to pipe_to_server.");
                _exit(1);
            }

            // Closes the pipe to the server
            res = close(fd_write);
            if (res == -1) {
                perror("Error closing pipe_to_server.");
                _exit(1);
            }
        }
        else {
            perror("Error in the number of arguments.");
            _exit(1);
        }
    }
    else {
        perror("Error invalid option.");
        _exit(1);
    }

    return 0;
}
