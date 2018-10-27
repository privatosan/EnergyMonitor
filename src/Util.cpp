#include "Util.h"

#include <sys/time.h>

timeValueUs time()
{
    struct timeval tv;

    gettimeofday(&tv, nullptr);

    return (timeValueUs)tv.tv_sec * 1000000 + (timeValueUs)tv.tv_usec;
}
