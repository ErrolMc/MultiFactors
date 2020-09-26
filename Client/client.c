#include "utils.h"

struct WorkData 
{
    int slot;
    struct SharedMemory *shmPTR;
};

struct timespec lastUpdateTime;
int canCheckProgress = 0;

void* ProgressThreadWorker(void *arg)
{
    struct WorkData *data = (struct WorkData *)arg;
    struct SharedMemory *shmPTR = data->shmPTR;

    struct timespec ts;
    while (1)
    {
        if (canCheckProgress)
        {
            TimerStart(&ts);
            long diff = TimerStop_ms(&lastUpdateTime);
            if (diff > 500)
            {
                int active = 0;
                for (int i = 0; i < INT_BITS; i++)
                {
                    if (shmPTR->slotStatus[i] == 1)
                    {
                        active = 1;
                        break;
                    }
                }

                if (active)
                {
                    printf("Progress: ");
                    for (int i = 0; i < NUM_SLOTS; i++)
                    {
                        int ind = i + 1;
                        if (shmPTR->slotStatus[i] == 1)
                        {
                            float progress = shmPTR->slotProgress[i];
                            float prog01 = progress / (float)INT_BITS;

                            int prog0100 = (int)(100.0 * prog01);
                            int progDisplay = (int)((float)PROG_DISPLAY_LENGTH * prog01);

                            printf("Q%d: %d\% |", ind, prog0100); // Q1: 50% |
                            BarDisplay(progDisplay, PROG_DISPLAY_LENGTH);
                            printf("| ");
                        }
                    }
                    printf("\n");

                    TimerStart(&lastUpdateTime);
                }
            }
            else
            {
                msleep(diff);
            }
        }
    }
}

void* ThreadWorker(void *arg)
{
    struct WorkData *data = (struct WorkData *)arg;
    int slot = data->slot;
    struct SharedMemory *shmPTR = data->shmPTR;

    struct timespec ts;
    TimerStart(&ts);

    int startInd = INT_BITS * slot;

    while (1)
    {
        if (shmPTR->serverFlag[slot] == 1)
        {
            int num = shmPTR->slot[slot];
            printf("slot: %d, factor: %d\n", slot, num);
            shmPTR->serverFlag[slot] = 0; // mark as read

            TimerStart(&lastUpdateTime);
            canCheckProgress = 1;
        }
        else if (shmPTR->slotStatus[slot] == 0) // we are done reading for this slot
            break;
    }

    canCheckProgress = 0;
    for (int i = 0; i < NUM_SLOTS; i++)
    {
        if (shmPTR->slotStatus[i] == 1)
        {
            canCheckProgress = 1;
            break;
        }
    }
    
    int timeTaken = TimerStop_s(&ts);
    printf("slot done: %d, time taken: %ds\n", slot, timeTaken);
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
    pthread_t progThread;

    // progress thread
    {
        struct WorkData data;
        data.shmPTR = shmPTR;

        pthread_create(&progThread, NULL, ProgressThreadWorker, (void*)&data);
        pthread_detach(progThread);
    }

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