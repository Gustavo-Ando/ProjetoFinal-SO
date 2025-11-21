#include <stdlib.h>
#include <stdio.h>

#include "utility.h"

/*
    Function to send an error message and terminate execution
    Params:
        - char *str: string to print
    Return:
        - 
*/
void fail(char *str) {
    perror(str);
    exit(EXIT_FAILURE);
}