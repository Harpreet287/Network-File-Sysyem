#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <stdio.h>
#include "./Trie.h"

// structure for clock object
typedef struct Clock
{
    double bootTime;
    struct timespec Btime;
}CLOCK;

CLOCK* InitClock();
double GetCurrTime(CLOCK* clock);

Trie* File_Trie;
extern FILE* Log_File;
extern CLOCK* Clock;

#endif // __HEADERS_H__