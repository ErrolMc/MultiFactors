#include "utils.h"

struct WorkData 
{
    int slot;
    struct SharedMemory *shmPTR;
};

void* ThreadWorker(void *arg)
{
    struct WorkData *data = (struct WorkData *)arg;
    int slot = data->slot;
    struct SharedMemory *shmPTR = data->shmPTR;

    int startInd = INT_BITS * slot;

    while (1)
    {
        if (shmPTR->serverFlag[slot] == 1)
        {
            int num = shmPTR->slot[slot];
            printf("slot: %d, factor: %d\n", slot, num);
            shmPTR->serverFlag[slot] = 0; // mark as read
        }
        else if (shmPTR->slotStatus[slot] == 0) // we are done reading for this slot
            break;
    }
}

int main() 
{
    key_t shmKEY;
    int shmID;
    struct SharedMemory *shmPTR;

    shmKEY = ftok("..", 'x');
    shmID = shmget(shmKEY, sizeof(struct SharedMemory), 0666);
    if (shmID < 0) 
    {
        printf("*** shmget error (client) ***\n");
        exit(1);
    }

    shmPTR = (struct SharedMemory *)shmat(shmID, NULL, 0);
    if ((int)shmPTR == -1) 
    {
        printf("*** shmat error (client) ***\n");
        exit(1);
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * NUM_SLOTS);

    while (1)
    {
        char buff[128];
        gets(buff);

        if (strcmp(buff, "quit") == 0)
        {
            shmPTR->active = 0;
            break;
        }

        int num = atoi(buff);

        shmPTR->number = num;
        shmPTR->clientFlag = 1;

        while (shmPTR->clientFlag == 1)
            ;

        int slot = shmPTR->number;
        if (slot != -1)
        {   
            // create a thread to listen for the responses for this query
            struct WorkData data;
            data.slot = slot;
            data.shmPTR = shmPTR;

            pthread_create(&threads[slot], NULL, ThreadWorker, (void*)&data);
            pthread_detach(threads[slot]);
        }
    }

    printf("client has detached its shared memory...\n");
    shmdt((void *)shmPTR);

    return 0;
}