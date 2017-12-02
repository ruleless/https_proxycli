#include "proxy_client.h"

NAMESPACE_BEG(proxy)

bool setNonblocking(int fd)
{
    int fstatus = fcntl(fd, F_GETFL);
    if (fstatus < 0)
        return false;
    
    if (fcntl(fd, F_SETFL, fstatus|O_NONBLOCK) < 0)
        return false;

    return true;
}

uint64 getClock64()
{
#if defined (__WIN32__) || defined(_WIN32) || defined(WIN32)
    return ::GetTickCount();
#elif defined(unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    
    uint64 value = ((uint64)time.tv_sec) * 1000 + (time.tv_usec/1000);  
    return value;
#else
# error Unsupported platform!
#endif
}

uint32 getClock()
{
    return (uint32) (getClock64() & 0xfffffffful);
}

NAMESPACE_END
