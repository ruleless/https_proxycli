#include "listener.h"

NAMESPACE_BEG(proxy)

Listener::~Listener()
{
    finalise();
}

bool Listener::initialise(const char *ip, int port)
{
    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &remoteAddr.sin_addr) < 0)
    {
        ErrorPrint("[Listener::initialise] illegal ip(%s)", ip);
        return false;
    }

    return initialise((const sockaddr *)&remoteAddr, sizeof(remoteAddr));
}

bool Listener::initialise(const sockaddr *sa, socklen_t salen)
{
    if (mFd >= 0)
    {
        ErrorPrint("Listener already inited!");
        return false;
    }

    mFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mFd < 0)
    {
        ErrorPrint("[Listener::initialise] init socket error! %s", strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(mFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

#ifdef SO_REUSEPORT
    opt = 1;
    setsockopt(mFd, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt));
#endif

    // set nonblocking
    if (!setNonblocking(mFd))
    {
        ErrorPrint("[Listener::initialise] set nonblocking error! %s", strerror(errno));
        goto err_1;
    }

    if (bind(mFd, sa, salen) < 0)
    {
        ErrorPrint("[Listener::initialise] bind error! %s", strerror(errno));
        goto err_1;
    }

    if (listen(mFd, LISTENQ) < 0)
    {
        ErrorPrint("[Listener::initialise] listen failed! %s", strerror(errno));
        goto err_1;
    }

    if (!mEventPoller->registerForRead(mFd, this))
    {
        ErrorPrint("[Listener::initialise] registerForRead failed! %s", strerror(errno));
        goto err_1;
    }

    return true;

err_1:
    close(mFd);
    mFd = -1;

    return false;
}

void Listener::finalise()
{
    if (mFd < 0)
        return;

    mEventPoller->deregisterForRead(mFd);
    close(mFd);
    mFd = -1;
}

int Listener::handleInputNotification(int fd)
{
    struct sockaddr_in addr;
    socklen_t addrlen;
    int newConns = 0;
    while (newConns++ < 32)
    {
        addrlen = sizeof(addr);
        int connfd = accept(fd, (sockaddr *)&addr, &addrlen);
        if (connfd < 0)
        {
            // DebugPrint("accept failed! %s", strerror(errno));
            break;
        }
        else
        {
#if 0
            struct sockaddr_in localAddr, remoteAddr;
            socklen_t localAddrLen = sizeof(localAddr), remoteAddrLen = sizeof(remoteAddr);
            if (getsockname(connfd, (sockaddr *)&localAddr, &localAddrLen) == 0 &&
                getpeername(connfd, (sockaddr *)&remoteAddr, &remoteAddrLen) == 0)
            {
                char remoteip[MAX_BUF] = {0}, localip[MAX_BUF] = {0};
                if (inet_ntop(AF_INET, &localAddr.sin_addr, localip, sizeof(localip)) &&
                    inet_ntop(AF_INET, &remoteAddr.sin_addr, remoteip, sizeof(remoteip)))
                {
                    char acceptLog[1024] = {0};
                    snprintf(acceptLog, sizeof(acceptLog), "(%s:%d) accept from %s:%d",
                             localip, ntohs(localAddr.sin_port),
                             remoteip, ntohs(remoteAddr.sin_port));
                    DebugPrint(acceptLog);
                }
            }
#endif

            if (mHandler)
            {
                mHandler->onAccept(connfd);
            }
        }
    }

    return 0;
}

NAMESPACE_END // namespace proxy
