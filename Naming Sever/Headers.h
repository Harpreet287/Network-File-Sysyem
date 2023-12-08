#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <stdio.h>
#include <sys/time.h>


#define MAX_QUEUE_SIZE 5

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

#endif