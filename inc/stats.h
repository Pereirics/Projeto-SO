#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


// Calculates the total time in the PIDs given by the user
void stats_time(char args[][7], char* folder) {

    int fd, res, pos;
    int bytes_read, bytes_written;
    int total = 0;
    char str[128];
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
            perror("Error opening file with PID name."); // This will happen when the user inserts a PID that doesn't exist
            return;
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
            return;
        }
    }

    // Opens the pipe to the client (Writing)
    fd = open("pipe_to_client", O_WRONLY);
    if (fd == -1) {
        perror("Error opening pipe_to_client.");
        return;
    }

    // Formats the string that will be sent to the server
    res = snprintf(str, sizeof(str), "Total execution time is %d ms\n", total);
    if (res < 0) {
        perror("Error formatting string.");
        return;
    }

    // Sends the string to the server
    bytes_written = write(fd, str, strlen(str)); 
    if (bytes_written == -1) {
        perror("Error writing to client.");
        return;
    }

    // Closes the pipe to the client
    res = close(fd);
    if (res == -1) {
        perror("Error closing pipe_to_client.");
        return;
    }
}

// Calculates the number of times a program given by the user was executed in the PIDs given by the user
void stats_command(char* cmd, char args[][7], char* folder) {

    int fd, res, pos;
    char c, str[128];
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
            perror("Error opening the file with PID name."); // This will happen when the user inserts a PID that doesn't exist
            return;
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
    char c, str[128];
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
            perror("Error opening file with PID name."); // This will happen when the user inserts a PID that doesn't exist
            return;
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
            return;
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