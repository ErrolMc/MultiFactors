#include "utils.h"

struct WorkData 
{
    int slot;
    int ind;
    struct SharedMemory *shmPTR;
};

static int thread_created_flag = 0;
int numbersToProcess[NUM_SLOTS * INT_BITS];
int numbersToProcessFlag[NUM_SLOTS * INT_BITS];
int startedFactorisingFlag[NUM_SLOTS * INT_BITS];
pthread_mutex_t mutexArr[NUM_SLOTS];

void SendFactor(int number, int factor, int slot, struct SharedMemory* shmPTR)
{
    //printf("found factor: %d, for: %d, in slot: %d\n", factor, number, slot);
    pthread_mutex_lock(&mutexArr[slot]); // first get the lock

    // wait until the server flag is 0
    while (shmPTR->serverFlag[slot] == 1)
        ;

    //printf("sending factor: %d, for: %d, in slot: %d\n", factor, number, slot);

    // server flag is now 0
    shmPTR->slot[slot] = factor;
    shmPTR->serverFlag[slot] = 1;

    pthread_mutex_unlock(&mutexArr[slot]); // release the lock
}

void FactoriseNumber(int number, int slot, struct SharedMemory* shmPTR)
{
    int bound = number / 2;
    for (int i = 1; i <= bound; i++)
    {
        // found factor
        if (number % i == 0)
        {
            SendFactor(number, i, slot, shmPTR);
        }
    }

    // send the number itself as a factor
    SendFactor(number, number, slot, shmPTR);

    // increase the progress
    pthread_mutex_lock(&mutexArr[slot]); // first get the lock
    shmPTR->slotProgress[slot]++;
    pthread_mutex_unlock(&mutexArr[slot]); // release the lock
}

void* ThreadWorker(void *arg)
{
    struct WorkData *data = (struct WorkData *)arg;
    int slot = data->slot;
    int ind = data->ind;
    struct SharedMemory *shmPTR = data->shmPTR;

    int startInd = INT_BITS * slot;
    int localInd = startInd + ind;
    int endInd = startInd + INT_BITS - 1;

    thread_created_flag = 0;

    while (1)
    {
        if (numbersToProcessFlag[localInd] == READY)
        {
            numbersToProcessFlag[localInd] = IN_PROGRESS;
            startedFactorisingFlag[localInd] = 1;

            int number = numbersToProcess[localInd];
            //printf("slot: %d, ind: %d, number: %d, localind: %d\n", slot, ind, number, localInd);
            FactoriseNumber(number, slot, shmPTR);
            //printf("done factoring slot: %d, ind: %d, number: %d, localind: %d\n", slot, ind, number, localInd);

            numbersToProcessFlag[localInd] = DONE;

            int done = 1;

            for (int i = startInd; i <= endInd; i++)
            {
                //printf("checking: %d, status: %d, slot: %d, ind: %d\n", i, numbersToProcessFlag[i], slot, ind);
                if (numbersToProcessFlag[i] == IN_PROGRESS || startedFactorisingFlag[i] == 0)
                {
                    done = 0;
                    break;
                }
            }

            if (done == 1)
            {
                for (int i = 0; i < INT_BITS; i++)
                {
                    int curInd = (INT_BITS * slot) + i;
                    startedFactorisingFlag[curInd] = 0;
                }
                
                shmPTR->slotStatus[slot] = 0;
                printf("slot done: %d\n", slot);
            }
        }
    }
}

int RotateNumber(int n, unsigned int d) 
{ 
    /* In n>>d, first d bits are 0.  
    To put last 3 bits of at  
    first, do bitwise or of n>>d 
    with n <<(INT_BITS - d) */
    return (n >> d)|(n << (INT_BITS - d)); 
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

    // init thread mutexes
    for (int i = 0; i < NUM_SLOTS; i++)
        pthread_mutex_init(&mutexArr[i], NULL);
    
    // create the threads
    pthread_t *threads = malloc(sizeof(pthread_t) * NUM_SLOTS * INT_BITS);

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
            while (thread_created_flag == 1)
                ;
        }
        printf("Created threads for slot %d\n", i);
    }

    while (1)
    {
        if (shmPTR->active == 0)
            break;

        if (shmPTR->clientFlag == 1)
        {
            int num = shmPTR->number;

            int freeSlot = -1;
            for (int i = 0; i < NUM_SLOTS; i++)
            {
                if (shmPTR->slotStatus[i] == 0)
                {
                    freeSlot = i;
                    break;
                }
            }

            shmPTR->number = freeSlot;
            

            if (freeSlot != -1)
            {
                shmPTR->slotStatus[freeSlot] = 1;
                shmPTR->slotProgress[freeSlot] = 0;
                
                printf("generating factors for: %d, on slot: %d\n", num, freeSlot);

                for (int i = 0; i < INT_BITS; i++)
                {
                    int curInd = (INT_BITS * freeSlot) + i;
                    int curNum = abs(RotateNumber(num, i));

                    numbersToProcess[curInd] = curNum;
                    numbersToProcessFlag[curInd] = READY;
                }
            }
            shmPTR->clientFlag = 0;
        }

        msleep(1);
    }

    for (int i = 0; i < NUM_SLOTS * INT_BITS; i++)
        pthread_cancel(threads[i]);

    shmdt((void *)shmPTR);
    printf("server has detached its shared memory...\n");
    shmctl(shmID, IPC_RMID, NULL);
    
    return 0;
}