#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <stdio.h>
#include <sys/time.h>
#include "./Server_Handle.h"


#define MAX_QUEUE_SIZE 5
#define MAX_PATH_LEN 1024
#define LOG_FLUSH_INTERVAL 10
#define CLOCK_MONOTONIC_RAW 4

// Command Codes
#define CMD_READ 1
#define CMD_WRITE 2
#define CMD_CREATE 3
#define CMD_DELETE 4
#define CMD_INFO 5
#define CMD_LIST 6
#define CMD_MOVE 7
#define CMD_COPY 8
#define CMD_RENAME 9


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

// Function to check if a socket is connected (Readable)
int IsSocketConnected(int sockfd);

// Function for path resolution
SERVER_HANDLE_STRUCT* ResolvePath(char* path);

#endif