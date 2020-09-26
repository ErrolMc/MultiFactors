#include "utils.h"

// Sleeps for an amount of milliseconds
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

// Rotates an integer n by d bits to the right
int RotateNumber(int n, unsigned int d) 
{ 
    return (n >> d)|(n << (INT_BITS - d)); 
} 
