#include "Externals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Path: External.c

void CheckError(int iStatus, char *sErrorMsg)
{
    if(iStatus < 0)
    {
        printf("Error: %s\n", sErrorMsg);
        perror(sErrorMsg);
        exit(EXIT_FAILURE);
    }
}

void CheckNull(void *ptr, char *sErrorMsg)
{
    if(ptr == NULL)
    {
        printf("Error: %s\n", sErrorMsg);
        perror(sErrorMsg);
        exit(EXIT_FAILURE);
    }
}

