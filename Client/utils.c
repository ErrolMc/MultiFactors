#include "utils.h"

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

void TimerStart(struct timespec *tp)
{
    clock_gettime(0, tp);
}

int TimerStop_s(struct timespec *start)
{
    struct timespec end;
    clock_gettime(0, &end);
    return end.tv_sec - start->tv_sec;
}

long TimerStop_ms(struct timespec *start)
{
    struct timespec end;
    clock_gettime(0, &end);

    long delta_ms = ((end.tv_sec - start->tv_sec) * 1000) + ((end.tv_nsec - start->tv_nsec) / 1000000);
    return delta_ms;
}

void BarDisplay(int full, int max)
{    
    for (int i = 0; i < full; i++) 
        printf("#"); 
    for (int i = 0; i < max-full; i++) 
        printf("_");    
}
