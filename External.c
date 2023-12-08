#include "Externals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Path: External.c

/**
 * Checks if the status is less than 0 and prints the error message.
 *
 * @param iStatus The status to be checked.
 * @param sErrorMsg The error message to be printed.
 * @return 1 if the status is less than 0, otherwise 0.
 */
int CheckError(int iStatus, char *sErrorMsg)
{
    if(iStatus < 0)
    {
        perror(sErrorMsg);
        return 1;
    }
    return 0;
}

/**
 * Checks if the pointer is NULL and prints the error message.
 *
 * @param ptr The pointer to be checked.
 * @param sErrorMsg The error message to be printed.
 * @return 1 if the pointer is NULL, otherwise 0.
 */
int CheckNull(void *ptr, char *sErrorMsg)
{
    if(ptr == NULL)
    {
        printf("Error: %s\n", sErrorMsg);
        perror(sErrorMsg);
        return 1;
    }
    return 0;
}

