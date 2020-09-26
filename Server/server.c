#include "utils.h"

// server global variables/flags
int thread_created_flag = 0;
int numbersToProcess[NUM_SLOTS * INT_BITS];
int numbersToProcessFlag[NUM_SLOTS * INT_BITS];
int startedFactorisingFlag[NUM_SLOTS * INT_BITS];
pthread_mutex_t mutexArr[NUM_SLOTS];

// Sends a factor back to the client through shared memory using a mutex
void SendFactor(int number, int factor, int slot, struct SharedMemory* shmPTR)
{
    pthread_mutex_lock(&mutexArr[slot]); // first get the lock

    // wait until the server flag is 0
    while (shmPTR->serverFlag[slot] == 1)
        ;

    // server flag is now 0
    shmPTR->slot[slot] = factor;
    shmPTR->serverFlag[slot] = 1;

    pthread_mutex_unlock(&mutexArr[slot]); // release the lock
}

// Factorises a number for a slot and sends the factors back through the shared memory
void FactoriseNumber(int number, int slot, struct SharedMemory* shmPTR)
{
    int bound = number / 2;
    for (int i = 1; i <= bound; i++)
    {
        // found factor
        if (number % i == 0)
            SendFactor(number, i, slot, shmPTR);
    }

    // send the number itself as a factor
    SendFactor(number, number, slot, shmPTR);

    // increase the progress
    pthread_mutex_lock(&mutexArr[slot]); // first get the lock
    shmPTR->slotProgress[slot]++;
    pthread_mutex_unlock(&mutexArr[slot]); // release the lock
}

// Thread that is responsable for calculating the factors for one of the numbers in a slot
void* ThreadWorker(void *arg)
{
    // first get the data sent through
    struct WorkData *data = (struct WorkData *)arg;
    int slot = data->slot;
    int ind = data->ind;
    struct SharedMemory *shmPTR = data->shmPTR;

    // calculate some indicies for referene
    int startInd = INT_BITS * slot;
    int localInd = startInd + ind;
    int endInd = startInd + INT_BITS - 1;

    // set the created flag to 0 so the system can create the next thread
    thread_created_flag = 0;

    while (1)
    {   
        // if the system is asking this thread to do some processing
        if (numbersToProcessFlag[localInd] == READY)
        {
            // set some flags to set this thread as in progress
            numbersToProcessFlag[localInd] = IN_PROGRESS;
            startedFactorisingFlag[localInd] = 1;

            // get the number and calculate the factors
            int number = numbersToProcess[localInd];
            FactoriseNumber(number, slot, shmPTR);

            // set this thread as done
            numbersToProcessFlag[localInd] = DONE;

            // see if all the other threads for this slot are done
            int done = 1;
            for (int i = startInd; i <= endInd; i++)
            {
                if (numbersToProcessFlag[i] == IN_PROGRESS || startedFactorisingFlag[i] == 0)
                {
                    done = 0;
                    break;
                }
            }

            // if all the threads for this slot are done, let the client know/reset some flags
            if (done == 1)
            {
                for (int i = startInd; i <= endInd; i++)
                    startedFactorisingFlag[i] = 0;
                
                shmPTR->slotStatus[slot] = 0;
                printf("slot done: %d\n", slot);
            }
        }
    }
}

// Loop that listens to requests from the client through the shared memory
void TalkToClient(struct SharedMemory* shmPTR)
{
    while (1)
    {
        // if the client has quit, quit
        if (shmPTR->active == 0)
            break;

        // if the client has sent a number through
        if (shmPTR->clientFlag == 1)
        {
            // get the number
            int num = shmPTR->number;

            // look for a free slot
            int freeSlot = -1;
            for (int i = 0; i < NUM_SLOTS; i++)
            {
                if (shmPTR->slotStatus[i] == 0)
                {
                    freeSlot = i;
                    break;
                }
            }

            // send the slot back to the client
            shmPTR->number = freeSlot;

            if (freeSlot != -1)
            {
                // init the slot in the shared memory
                shmPTR->slotStatus[freeSlot] = 1;
                shmPTR->slotProgress[freeSlot] = 0;
                
                printf("generating factors for: %d, on slot: %d\n", num, freeSlot);

                // generate the rotations for the number and distribute it to the appropriate threads
                for (int i = 0; i < INT_BITS; i++)
                {
                    int curInd = (INT_BITS * freeSlot) + i; // get the index where the number is going to go
                    int curNum = abs(RotateNumber(num, i)); // get the number based on the rotation

                    numbersToProcess[curInd] = curNum;      // set the number
                    numbersToProcessFlag[curInd] = READY;   // let the appropriate thread know that it is ready to process the number
                }
            }

            // let the client read the slot
            shmPTR->clientFlag = 0;
        }

        msleep(1);
    }
}

int main() 
{
    // variables for shared memory/threads
    key_t shmKEY;
    int shmID;
    struct SharedMemory *shmPTR;
    pthread_t *threads = malloc(sizeof(pthread_t) * NUM_SLOTS * INT_BITS);

    // get the shared memory id
    shmKEY = ftok("..", 'x');
    shmID = shmget(shmKEY, sizeof(struct SharedMemory), IPC_CREAT | 0666);
    if (shmID < 0) 
    {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    // get the shared memory pointer
    shmPTR = (struct SharedMemory *)shmat(shmID, NULL, 0);
    if ((int)shmPTR == -1) 
    {
        printf("*** shmat error (server) ***\n");
        exit(1);
    }

    // init some variables in the shared memory
    shmPTR->clientFlag = 0;
    shmPTR->active = 0;

    // init thread mutexes
    for (int i = 0; i < NUM_SLOTS; i++)
        pthread_mutex_init(&mutexArr[i], NULL);
    
    // create the threads
    for (int i = 0, ind = 0; i < NUM_SLOTS; i++)
    {
        for (int j = 0; j < INT_BITS; j++, ind++)
        {
            struct WorkData data;
            data.slot = i;
            data.ind = j;
            data.shmPTR = shmPTR;

            thread_created_flag = 1;
            pthread_create(&threads[ind], NULL, ThreadWorker, (void*)&data);
            while (thread_created_flag == 1) // wait till the current thread has been created before creating the next one
                ;
        }
        printf("Created threads for slot %d\n", i);
    }

    // mark the server as ready
    shmPTR->active = 1;
    printf("All the threads have been created\n");

    TalkToClient(shmPTR);

    // cancel all the threads
    for (int i = 0; i < NUM_SLOTS * INT_BITS; i++)
        pthread_cancel(threads[i]);

    // detatch the shared memory
    shmdt((void *)shmPTR);
    printf("server has detached its shared memory...\n");
    shmctl(shmID, IPC_RMID, NULL);
    
    return 0;
}