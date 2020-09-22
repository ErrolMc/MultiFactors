#include "utils.h"

struct WorkData 
{
    int slot;
};

void* ThreadWorker(void *arg)
{
    struct WorkData *data = (struct WorkData *)arg;
    int slot = data->slot;

    while (1)
    {

    }
}

int main() 
{ 
    key_t shmKEY;
    int shmID;
    struct SharedMemory *shmPTR;

    shmKEY = ftok("..", 'x');
    shmID = shmget(shmKEY, sizeof(struct SharedMemory), IPC_CREAT | 0666);
    if (shmID < 0) 
    {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    shmPTR = (struct SharedMemory *)shmat(shmID, NULL, 0);
    if ((int)shmPTR == -1) 
    {
        printf("*** shmat error (server) ***\n");
        exit(1);
    }

    shmPTR->clientFlag = 0;
    shmPTR->active = 1;
    for (int i = 0; i < NUM_SLOTS; i++)
        shmPTR->serverFlag[i] = 0;

    while (1)
    {
        if (shmPTR->active == 0)
            break;

        if (shmPTR->clientFlag == 1)
        {
            int num = shmPTR->number;
            printf("Num: %d\n", num);

            shmPTR->clientFlag = 0;
        }

        msleep(1);
    }

    shmdt((void *)shmPTR);
    printf("server has detached its shared memory...\n");
    shmctl(shmID, IPC_RMID, NULL);
    

    // create threads
    /*
    pthread_t *threads = malloc(sizeof(pthread_t) * INT_BITS);

    for (int i = 0; i < NUM_SLOTS; i++)
    {
        for (int j = 0; j < INT_BITS; j++)
        {
            struct WorkData data;
            data.slot = i;

            msleep(10);

            pthread_create(&threads[i], NULL, ThreadWorker, (void*)&data);
        }
    }
    */

    return 0;
}