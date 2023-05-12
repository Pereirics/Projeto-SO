#include <sys/time.h>

typedef struct prog 
{
    int pid;
    char cmd[256];
    char args[256][7];
    struct timeval start;
    int ms;
} prog;