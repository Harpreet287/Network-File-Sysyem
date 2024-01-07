#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <stdio.h>
#include "./Trie.h"

#define LOG_FLUSH_INTERVAL 10

// structure for clock object
typedef struct Clock
{
    double bootTime;
    struct timespec Btime;
}CLOCK;

CLOCK* InitClock();
double GetCurrTime(CLOCK* clock);

// Trie* File_Trie;
// int NS_Write_Socket;

extern FILE* Log_File;
extern CLOCK* Clock;

// Thread to listen for NS requests
void* NS_Listner_Thread(void* arg);
// Thread to listen for client requests
void* Client_Listner_Thread(void* arg);

// Populates the File_Trie with the contents of the cwd
Trie* Initialize_File_Trie();

#endif // __HEADERS_H__