#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_TOKENS 1000

int main(int argc, char** argv) {

    int res, fd1, fd2, bytes_read;
    char* buffer[4096];

    res = mkfifo("pipe", 0666);
    
    fd1 = open("pipe", O_RDONLY);
    fd2 = open("pipe", O_WRONLY); // Mantém o servidor a correr

    while ((bytes_read = read(fd1, buffer, sizeof(buffer))) > 0) {
        write(1, buffer, bytes_read); 
    }

    close(fd1);
    close(fd2);
    unlink("pipe");

    return 0;
}