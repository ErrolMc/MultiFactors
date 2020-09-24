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

int TimerStop(struct timespec *start)
{
    struct timespec end;
    clock_gettime(0, &end);
    return end.tv_sec - start->tv_sec;
}