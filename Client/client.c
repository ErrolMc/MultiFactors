#include "utils.h"

// client global variables/flags
struct timespec lastUpdateTime;
int canCheckProgress = 0;

// Thread that looks for and logs progress
void* ProgressThreadWorker(void *arg)
{
    struct WorkData *data = (struct WorkData *)arg;
    struct SharedMemory *shmPTR = data->shmPTR;

    struct timespec ts;
    while (1)
    {
        // only check if there are any threads running
        if (canCheckProgress)
        {
            // see if there has been a 500ms diff since we last got an update
            TimerStart(&ts);
            long diff = TimerStop_ms(&lastUpdateTime);
            if (diff > 500)
            {
                printf("Progress: ");

                // create the progress bar for each of the slots that are active
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
            else
            {
                msleep(diff);
            }
        }
    }
}

// Thread that manages data that goes through a single slot
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

// Loop that listens for inputs then sends them back to the server
void TalkToServer(struct SharedMemory* shmPTR)
{
    pthread_t *threads = malloc(sizeof(pthread_t) * NUM_SLOTS);

    // main loop which looks for inputs
    while (1)
    {
        // get an input
        char buff[128];
        gets(buff);

        // see if the server is ready
        if (shmPTR->active == 1)
        {
            // if we have quit, tell the server to quit
            if (strcmp(buff, "quit") == 0)
            {
                shmPTR->active = 0;
                break;
            }

            // get the number and send it to the server
            int num = atoi(buff);

            shmPTR->number = num;
            shmPTR->clientFlag = 1;

            // wait for the server to process the number
            while (shmPTR->clientFlag == 1)
                ;

            // now that the number is processed, get the slot
            int slot = shmPTR->number;
            if (slot != -1)
            {   
                // create a thread to listen for the responses for this slot
                struct WorkData data;
                data.slot = slot;
                data.shmPTR = shmPTR;

                pthread_create(&threads[slot], NULL, ThreadWorker, (void*)&data);
                pthread_detach(threads[slot]);
            }
            else
            {
                printf("Cant create slot, please wait for a slot to become available\n");
            }
        }
        else
        {
            printf("Please wait until the server has created all its threads\n");
        }
    }
}

int main() 
{
    // variables for shared memory/threads
    key_t shmKEY;
    int shmID;
    struct SharedMemory *shmPTR;

    // get the shared memory id
    shmKEY = ftok("..", 'x');
    shmID = shmget(shmKEY, sizeof(struct SharedMemory), 0666);
    if (shmID < 0) 
    {
        printf("*** shmget error (client) ***\n");
        exit(1);
    }

    // get the shared memory pointer
    shmPTR = (struct SharedMemory *)shmat(shmID, NULL, 0);
    if ((int)shmPTR == -1) 
    {
        printf("*** shmat error (client) ***\n");
        exit(1);
    }

    // create the progress checker thread
    pthread_t progThread;
    struct WorkData data;
    data.shmPTR = shmPTR;

    pthread_create(&progThread, NULL, ProgressThreadWorker, (void*)&data);
    pthread_detach(progThread);
    
    // start talking to the server and listening for inputs
    TalkToServer(shmPTR);

    // detatch shared memory
    printf("client has detached its shared memory...\n");
    shmdt((void *)shmPTR);

    return 0;
}