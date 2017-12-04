#include "proxy_client.h"

#include "select_poller.h"

NAMESPACE_BEG(proxy)

ProxyClient gProxyClient;

ProxyClient::~ProxyClient()
{}

bool ProxyClient::initialise(const char *ip, int port)
{
    if (mInited)
    {
        return true;
    }

    mEventPoller = new SelectPoller();
    assert(mEventPoller && "alloc event poller failed.");

    mListener = new Listener(mEventPoller);
    assert(mListener && "alloc listener failed.");

    if (!mListener->initialise(ip, port))
    {
        ErrorPrint("bind %s:d failed.", ip, port);

        delete mListener;
        mListener = NULL;

        delete mEventPoller;
        mEventPoller = NULL;

        return false;
    }
    mListener->setEventHandler(this);

    mInited = true;
    return true;
}

void ProxyClient::finalise()
{
    if (!mInited)
    {
        return;
    }
    mInited = false;

    TunnelSet::iterator itSet;
    for (itSet = mBrokenTuns.begin(); itSet != mBrokenTuns.end(); itSet++)
    {
        delete *itSet;
    }
    mBrokenTuns.clear();

    TunnelList::iterator it;
    for (it = mFreeTuns.begin(); it != mFreeTuns.end(); it++)
    {
        delete *it;
    }
    mFreeTuns.clear();

    mListener->finalise();

    delete mListener;
    mListener = NULL;

    delete mEventPoller;
    mEventPoller = NULL;
}

bool ProxyClient::setDestServer(const char *ip, int port)
{
    struct sockaddr_in tmpaddr;

    if (inet_pton(AF_INET, ip, &tmpaddr.sin_addr) < 0)
    {
        ErrorPrint("[ProxyClient::setDestServer] illegal ip(%s).", ip);
        return false;
    }

    snprintf(mDestIp, sizeof(mDestIp), "%s", ip);
    mDestPort = port;

    return true;
}

bool ProxyClient::setProxyServer(const char *ip, int port)
{
    struct sockaddr_in tmpaddr;

    if (inet_pton(AF_INET, ip, &tmpaddr.sin_addr) < 0)
    {
        ErrorPrint("[ProxyClient::setProxyServer] illegal ip(%s).", ip);
        return false;
    }

    snprintf(mProxyIp, sizeof(mProxyIp), "%s", ip);
    mProxyPort = port;

    return true;
}

void ProxyClient::runLoop()
{
    mbLoop = true;

    while (mbLoop)
    {
        // 事件分发
        mEventPoller->processPendingEvents(PER_FRAME_TIME);

        // 回收断开的隧道
        TunnelSet::iterator it = mBrokenTuns.begin();
        for (; it != mBrokenTuns.end(); it++)
        {
            reclaimTunnel(*it);
        }
        mBrokenTuns.clear();
    }
}

void ProxyClient::exitLoop()
{
    mbLoop = false;
}

void ProxyClient::onAccept(int connfd)
{
    ProxyTunnel *tun = newTunnel();
    if (!tun)
    {
        ErrorPrint("[ProxyClient::onAccept] create tunnel failed. fd=%d", connfd);
        close(connfd);
        return;
    }

    if (!tun->setDestServer(mDestIp, mDestPort))
    {
        ErrorPrint("[ProxyClient::onAccept] set dest server failed(%s:%d). fd=%d",
                   mDestIp, mDestPort, connfd);
        close(connfd);
        reclaimTunnel(tun);
        return;
    }

    if (!tun->setProxyServer(mProxyIp, mProxyPort))
    {
        ErrorPrint("[ProxyClient::onAccept] set proxy server failed(%s:%d). fd=%d",
                   mProxyIp, mProxyPort, connfd);
        close(connfd);
        reclaimTunnel(tun);
        return;
    }

    if (!tun->acceptLocal(connfd))
    {
        ErrorPrint("[ProxyClient::onAccept] accept local conn failed. fd=%d", connfd);
        close(connfd);
        mBrokenTuns.insert(tun);
        return;
    }

    DebugPrint("[ProxyClient::onAccept] tun:%p created", tun);
    tun->setHandler(this);
}

void ProxyClient::onClosed(ProxyTunnel *tun)
{
    DebugPrint("[ProxyClient::onClosed] tun:%p closed", tun);
    tun->cleanup();

    mBrokenTuns.insert(tun);
}

void ProxyClient::onError(ProxyTunnel *tun)
{
    WarningPrint("[ProxyClient::onError] tun:%p error", tun);
    tun->cleanup();

    mBrokenTuns.insert(tun);
}

ProxyTunnel *ProxyClient::newTunnel()
{
    if (mFreeTuns.empty())
    {
        return new ProxyTunnel(mEventPoller);
    }

    ProxyTunnel *t = mFreeTuns.front(); assert(t && "get from free tunlist");
    mFreeTuns.pop_front();

    return t;
}

void ProxyClient::reclaimTunnel(ProxyTunnel *tun)
{
    if (!tun)
    {
        return;
    }

    if (mFreeTuns.size() >= CACHE_TUN_SIZE)
    {
        delete tun;
        return;
    }

    mFreeTuns.push_back(tun);
}

NAMESPACE_END // proxy
