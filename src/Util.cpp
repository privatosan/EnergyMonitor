#include "Util.h"

#include <sys/time.h>

timeValueMs time()
{
    struct timeval tv;

    gettimeofday(&tv, nullptr);

    return (timeValueMs)tv.tv_sec * 1000 + (timeValueMs)tv.tv_usec / 1000;
}
