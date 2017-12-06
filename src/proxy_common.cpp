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

/*
 * Base-64 encoding.  This encodes binary data as printable ASCII characters.
 * Three 8-bit binary bytes are turned into four 6-bit values, like so:
 *
 *   [11111111]  [22222222]  [33333333]
 *
 *   [111111] [112222] [222233] [333333]
 *
 * Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
 */

static char B64_ENCODE_TABLE[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',  /* 0-7 */
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',  /* 8-15 */
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  /* 16-23 */
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',  /* 24-31 */
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',  /* 32-39 */
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  /* 40-47 */
    'w', 'x', 'y', 'z', '0', '1', '2', '3',  /* 48-55 */
    '4', '5', '6', '7', '8', '9', '+', '/'   /* 56-63 */
};

static int B64_DECODE_TABLE[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

/*
 * Do base-64 encoding on a hunk of bytes.   Return the actual number of
 * bytes generated.  Base-64 encoding takes up 4/3 the space of the original,
 * plus a bit for end-padding.  3/2+5 gives a safe margin.
 */
int Base64Encode(const unsigned char *ptr, int len, unsigned char *space, int size)
{
    int ptr_idx, space_idx, phase;
    char c;

    if (len < 0 || size < 0)
        return -1;
    else if (0 == len)
        return 0;

    space_idx = 0;
    phase = 0;
    for (ptr_idx = 0; ptr_idx < len; ++ptr_idx)
    {
        switch (phase)
        {
        case 0:
            c = B64_ENCODE_TABLE[ptr[ptr_idx] >> 2];
            if (space_idx < size)
                space[space_idx++] = c;
            else
                return -1;
            c = B64_ENCODE_TABLE[(ptr[ptr_idx] & 0x3) << 4];
            if (space_idx < size)
                space[space_idx++] = c;
            else
                return -1;
            ++phase;
            break;
        case 1:
            space[space_idx - 1] = B64_ENCODE_TABLE[B64_DECODE_TABLE[space[space_idx - 1]] | (ptr[ptr_idx] >> 4)];
            c = B64_ENCODE_TABLE[(ptr[ptr_idx] & 0xf) << 2];
            if (space_idx < size)
                space[space_idx++] = c;
            else
                return -1;
            ++phase;
            break;
        case 2:
            space[space_idx - 1] = B64_ENCODE_TABLE[B64_DECODE_TABLE[space[space_idx - 1]] | (ptr[ptr_idx] >> 6)];
            c = B64_ENCODE_TABLE[ptr[ptr_idx] & 0x3f];
            if (space_idx < size)
                space[space_idx++] = c;
            else
                return -1;
            phase = 0;
            break;
        }
    }

    /* Pad with ='s. */
    while (phase++ < 3)
    {
        if (space_idx < size)
            space[space_idx++] = '=';
        else
            return -1;
    }

    return space_idx;
}

/*
 * Do base-64 decoding on a string.  Ignore any non-base64 bytes.
 * Return the actual number of bytes generated.  The decoded size will
 * be at most 3/4 the size of the encoded, and may be smaller if there
 * are padding characters (blanks, newlines).
 */
int Base64Decode(const unsigned char *in, int inlen, unsigned char *space, int size)
{
    const char *cp;
    int space_idx, phase;
    int d, prev_d = 0;
    unsigned char c;
    int i = 0;

    if (inlen < 0 || size < 0)
        return -1;
    else if (0 == inlen)
        return 0;

    space_idx = 0;
    phase = 0;

    for (cp = (char *)in; i < inlen; ++cp, ++i)
    {
        d = B64_DECODE_TABLE[(int) *cp];
        if (d != -1)
        {
            switch (phase)
            {
            case 0:
                ++phase;
                break;
            case 1:
                c = (unsigned char)((prev_d << 2) | ((d & 0x30) >> 4));
                if (space_idx < size)
                    space[space_idx++] = c;
                else
                    return -1;
                ++phase;
                break;
            case 2:
                c = (unsigned char)(((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
                if (space_idx < size)
                    space[space_idx++] = c;
                else
                    return -1;
                ++phase;
                break;
            case 3:
                c = (unsigned char)(((prev_d & 0x03) << 6) | d);
                if (space_idx < size)
                    space[space_idx++] = c;
                else
                    return -1;
                phase = 0;
                break;
            }
            prev_d = d;
        }
    }
    return space_idx;
}

NAMESPACE_END
