#ifndef __PROXY_CLIENT_H__
#define __PROXY_CLIENT_H__

#include "proxy_common.h"
#include "listener.h"
#include "proxy_tunnel.h"

#define PER_FRAME_TIME 1 // 每个逻辑帧最多停留1s
#define CACHE_TUN_SIZE 64

NAMESPACE_BEG(proxy)

class ProxyClient : public Listener::Handler, public ProxyTunnel::Handler
{
    typedef std::list<ProxyTunnel *> TunnelList;
    typedef std::set<ProxyTunnel *> TunnelSet;
  public:
    ProxyClient():mEventPoller(NULL)
                 ,mListener(NULL)
                 ,mInited(false)
                 ,mbLoop(false)
                 ,mFreeTuns()
                 ,mBrokenTuns()
    {
        *mDestIp = '\0';
        mDestPort = 0;
        *mProxyIp = '\0';
        mProxyPort = 0;
    }

    virtual ~ProxyClient();

    bool initialise(const char *ip, int port);
    void finalise();

    bool setDestServer(const char *ip, int port);
    bool setProxyServer(const char *ip, int port);

    void runLoop();
    void exitLoop();

    virtual void onAccept(int connfd);

    virtual void onClosed(ProxyTunnel *tun);
    virtual void onError(ProxyTunnel *tun);

  private:
    ProxyTunnel *newTunnel();
    void reclaimTunnel(ProxyTunnel *tun);

  private:
    EventPoller *mEventPoller;
    Listener *mListener;

    bool mInited;
    bool mbLoop;

    TunnelList mFreeTuns; // 空闲代理隧道
    TunnelSet mBrokenTuns; // 已断开的代理隧道

    char mDestIp[IPv4_SIZE];
    int mDestPort;

    char mProxyIp[IPv4_SIZE];
    int mProxyPort;
};

extern ProxyClient gProxyClient;

NAMESPACE_END // proxy

#endif // __PROXY_CLIENT_H__
