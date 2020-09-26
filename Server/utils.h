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
    int ind;
    struct SharedMemory *shmPTR;
};

int msleep(long msec);
int RotateNumber(int n, unsigned int d);

#endif