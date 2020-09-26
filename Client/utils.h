#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h> 
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <time.h>

#define INT_BITS 32
#define NUM_SLOTS 10
#define PROG_DISPLAY_LENGTH 10

struct SharedMemory
{
    int clientFlag;             // 0 when empty, set to 1 when the server should pull data from number     
    int serverFlag[NUM_SLOTS];  // 0 when free, 1 when ready for the client to read, client sets to 0 to indicate its been read
    
    int number; 
    int slot[NUM_SLOTS];
    int slotStatus[NUM_SLOTS];
    int slotProgress[NUM_SLOTS];

    int active;
};

int msleep(long msec);
void TimerStart(struct timespec *tp);
int TimerStop_s(struct timespec *start);
long TimerStop_ms(struct timespec *start);
void BarDisplay(int full, int max);

#endif