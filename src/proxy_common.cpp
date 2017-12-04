#include "proxy_common.h"

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

char *strstrICase(const char *strStart, const char *strEnd, const char *substr)
{
	const char *p1 = strStart, *p2 = strStart, *q = NULL;
	int IsSucceed = 0;
	int tmpDiff = 'a' - 'A';

	while ((p1 < strEnd) && !IsSucceed)
	{
		q = substr;
		if (*p1 >= 'a' && *p1<='z')
		{
			tmpDiff = -32;
		}
		else if(*p1>= 'A' && *p1 <= 'Z')
		{
			tmpDiff = 32;
		}
		else
		{
			tmpDiff = 0;
		}

		while (((*p1 == *q)|| (*p1) + tmpDiff == *q) && (*q != 0x00) && (p1 < strEnd))
		{
			p1++;
			q++;
			if(*p1 >= 'a' && *p1<='z')
			{
				tmpDiff = -32;
			}
			else if(*p1>= 'A' && *p1 <= 'Z')
			{
				tmpDiff = 32;
			}
			else
			{
				tmpDiff = 0;
			}
		}

		if (*q == 0x00)
		{
			IsSucceed = 1;

			return (char *)p2;
		}

		p1 = ++p2;
	}

	return NULL;
}

bool isValidIp(const char *ip)
{
    if (!ip || !*ip)
    {
        return false;
    }

    struct sockaddr_in tmpaddr;
    if (inet_pton(AF_INET, ip, &tmpaddr.sin_addr) < 0)
    {
        return false;
    }

    return true;
}

bool isValidPort(int port)
{
    return port > 0 && port <= 65535;
}

NAMESPACE_END
