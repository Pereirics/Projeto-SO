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

// Function that transforms a string into an array of strings with the chosen delimiter
int tokenize(char* comand, char **store, char* sep) {
    int token_number = 0;
    char *token = NULL;

    // Stores string in token and then stores in the array
    token = strsep(&comand, sep); 
    while (token != NULL && token_number < MAX_TOKENS) {
        if (!strcmp("\0", token))
            store[token_number] = NULL;
        else {
            store[token_number] = token;
            token_number++;
        }
        token = strsep(&comand, sep);
        
    }

    // Sets the last argument of the array as NULL
    store[token_number] = NULL;

    return token_number;
}

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

        // Stores the PID in the struct that will be sent to the parent process and to the server.
        par.pid = child_pid;
        server.pid = child_pid;

        // Stores the name of the program that will be run on the struct that will be sent to the parent process
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

        // Sends the struct with the PID, the name and start time of the program to the parent process
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
            else
                // Closes the reading file descriptor of the last pipe
                res = close(pipes[i-1][0]);
                if (res == -1) {
                    perror("Error closing the reading file descriptor of the pipe.");
                    _exit(1);
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


int main(int argc, char **argv) {
    int fd_write, fd_read, bytes_read, bytes_written, status, res;
    char *store[MAX_TOKENS];
    char cmd[MAX_TOKENS];
    char buffer[100];
    struct prog st;

    // Executes option "execute -u" that executes a program given by the user
    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        if (argc == 4) {
            // Stores the name of the program to be sent to the server
            strcpy(cmd, argv[3]);

            // Tokenizes the command with the delimiter ' ' and stores it in the string array store
            tokenize(argv[3], store, " ");

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
            char buffer[10];
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
            char buffer[10];
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
