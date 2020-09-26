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

#include <math.h>

#define DONE 0
#define IN_PROGRESS 1
#define READY 2 

#define INT_BITS 32
#define NUM_SLOTS 10

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

#endif