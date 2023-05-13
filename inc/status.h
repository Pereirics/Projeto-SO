#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "prog.h"

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