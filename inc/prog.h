#include <sys/time.h>

#ifndef PROG_H
#define PROG_H

typedef struct prog 
{
    int pid;
    char cmd[256];
    char args[256][7];
    struct timeval start;
    int ms;
} prog;

#endif /* PROG_H */
