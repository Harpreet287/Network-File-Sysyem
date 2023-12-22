#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "./Headers.h"
#include "./Trie.h"
#include "../Externals.h"
#include "../colour.h"

FILE *Log_File;
Trie* File_Trie;
CLOCK* Clock;


/**
 * @brief Initializes the clock object.
 * @return: A pointer to the clock object on success, NULL on failure.
**/
CLOCK* InitClock()
{
    CLOCK* C = (CLOCK*) malloc(sizeof(CLOCK));
    if(CheckNull(C, "[-]InitClock: Error in allocating memory"))
    {
        fprintf(Log_File, "[-]InitClock: Error in allocating memory \n");
        exit(EXIT_FAILURE);
    }
    
    C->bootTime = 0;
    C->bootTime = GetCurrTime(C);
    if(CheckError(C->bootTime, "[-]InitClock: Error in getting current time"))
    {
        fprintf(Log_File, "[-]InitClock: Error in getting current time\n");
        free(C);
        exit(EXIT_FAILURE);
    }

    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &C->Btime);
    if(CheckError(err, "[-]InitClock: Error in getting current time"))
    {
        fprintf(Log_File, "[-]InitClock: Error in getting current time\n");
        free(C);
        exit(EXIT_FAILURE);
    }
        
    return C;
}

/**
 * Returns the current time in seconds.
 * @param Clock: The clock object.
 * @return: The current time in seconds on success, -1 on failure.
**/
double GetCurrTime(CLOCK* Clock)
{
    if(CheckNull(Clock, "[-]GetCurrTime: Invalid clock object"))
    {
        fprintf(Log_File, "[-]GetCurrTime: Invalid clock object\n");
        return -1;
    }
    struct timespec time;
    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    if(CheckError(err, "[-]GetCurrTime: Error in getting current time"))
    {
        fprintf(Log_File, "[-]GetCurrTime: Error in getting current time\n");
        return -1;
    }
    return (time.tv_sec + time.tv_nsec * 1e-9) - (Clock->bootTime);
}

int main() {
  printf("Enter (2)Port Number You want to use for Communication:\t");
  int NSPort, ClientPort;
  scanf("%d %d", &NSPort, &ClientPort);

  // Initialize trie for storing and saving all files exposed by the server (includes entire cwd structure)
  // If a folder is exposed, then it's children are also exposed
  // If a file is exposed, then it's path is stored in the trie
  File_Trie = Initialize_File_Trie();
  
  // Initialize the log file
  Log_File = fopen("./SSlog.txt", "w");
  if(Log_File == NULL)
  {
    fprintf(stderr, "Error opening log file\n");
    exit(1);
  }
  
  // Initialize the clock
  Clock = InitClock();
  if(CheckNull(Clock, "[-]main: Error in initializing clock"))
  {
    fprintf(Log_File, "[-]main: Error in initializing clock [Time Stamp: %f]\n",GetCurrTime(Clock));
    exit(EXIT_FAILURE);
  }
  fprintf(Log_File, "[+]Server Initialized [Time Stamp: %f]\n",GetCurrTime(Clock));

  // Setup Listner for Name Server

  // Setup Listner for Client

  // Send Init Packet to Naming Server
}
