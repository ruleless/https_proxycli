#ifndef __PROXY_TUNNEL_H__
#define __PROXY_TUNNEL_H__

#include "proxy_client.h"
#include "event_poller.h"
#include "connection.h"

NAMESPACE_BEG(proxy)

class ProxyTunnel : public Connection::Handler
{
    enum EProxyStatus // 当前代理连接状态
    {
        ProxyStatus_Closed = 0,
        ProxyStatus_Error,
        ProxyStatus_Connecting,
        ProxyStatus_Connected,
    };

  public:
    class Handler
    {
      public:
        Handler();

        virtual void onClosed(ProxyTunnel *tun) = 0;
        virtual void onError(ProxyTunnel *tun) = 0;
    };
    
    ProxyTunnel(EventPoller *poller)
            :mEventPoller(poller)
            ,mHandler(NULL)
            ,mLocalConn(poller)             
            ,mProxyConn(poller)
            ,mProxyStatus(ProxyStatus_Closed)
    {
        memset(&mProxySvrAdd, 0, sizeof(mProxySvrAdd));
        *mDestSvrIp = '\0';
        mDestSvrPort = 0;
    }

    virtual ~ProxyTunnel();

    // 从本地客户端接入连接
    bool acceptLocal(int connfd);

    bool setProxyServer(const char *ip, int port);
    bool setDestServer(const char *ip, int port);

    virtual void onConnected(Connection *pConn);
    virtual void onDisconnected(Connection *pConn);

    virtual void onRecv(Connection *pConn, const void *data, size_t datalen);
    virtual void onError(Connection *pConn);

  private:
    // 跟代理服务器建立连接
    bool connectProxy();

  private:
    EventPoller *mEventPoller;

    Handler *mHandler;

    Connection mLocalConn;
    Connection mProxyConn;

    sockaddr_in mProxySvrAddr; // 代理服务器地址
    char mDestSvrIp[IPv4_SIZE]; // 目标服务器IP
    int mDestSvrPort; // 目标服务器端口

    EProxyStatus mProxyStatus;
};

NAMESPACE_END // proxy

#endif // __PROXY_TUNNEL_H__
