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