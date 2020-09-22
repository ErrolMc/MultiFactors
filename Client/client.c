#include "utils.h"

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

    while (1)
    {
        char buff[128];
        printf(" > ");
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
        {
            // waiting for client flag to be set back to 1
        }
    }

    printf("client has detached its shared memory...\n");
    shmdt((void *)shmPTR);

    return 0;
}