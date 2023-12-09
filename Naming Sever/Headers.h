#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <stdio.h>
#include <sys/time.h>


#define MAX_QUEUE_SIZE 5
#define MAX_PATH_LEN 1024
#define LOG_FLUSH_INTERVAL 10
#define CLOCK_MONOTONIC_RAW 4

// structure for clock object
typedef struct Clock
{
    double bootTime;
    struct timespec Btime;
}CLOCK;

CLOCK* InitClock();
double GetCurrTime(CLOCK* clock);

// global variables
extern FILE *logs;
extern CLOCK* Clock;

// Thread to Asynchronously accept client connections
void* Client_Acceptor_Thread();
// Thread to handle a client
void* Client_Handler_Thread(void* clientHandle);

// Thread to Asynchronously accept Storage Server connections
void* Storage_Server_Acceptor_Thread();
// Thread to handle a Storage Server
void* Storage_Server_Handler_Thread(void* storageServerHandle);

// Thread to Asynchronously flush the logs periodically
void* Log_Flusher_Thread();

// Function to handle server exit
void exit_handler();

#endif