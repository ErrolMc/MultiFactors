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
    int clientFlag;
    int serverFlag[NUM_SLOTS];
    
    int number; 
    int slot[NUM_SLOTS];
    int slotStatus[NUM_SLOTS];
    int slotProgress[NUM_SLOTS];

    int active;
};

struct WorkData 
{
    int slot;
    struct SharedMemory *shmPTR;
};

int msleep(long msec);
void TimerStart(struct timespec *tp);
int TimerStop_s(struct timespec *start);
long TimerStop_ms(struct timespec *start);
void BarDisplay(int full, int max);

#endif