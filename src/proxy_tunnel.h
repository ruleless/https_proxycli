#ifndef __PROXY_TUNNEL_H__
#define __PROXY_TUNNEL_H__

#include "proxy_common.h"
#include "event_poller.h"
#include "connection.h"
#include "cache.h"

#define HTTP_HEADER_SIZE           1024

#define HTTP_METHOD_CONNECT        "CONNECT %s:%d HTTP/1.1"
#define HTTP_FIELD_HOST            "Host: %s:%d"

#define SSL_CONNECTION_RESPONSE    "HTTP/1.0 200 Connection established"
#define SSL_CONNECTION_RESPONSE_OK "200 Connection established"

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

    typedef Cache<ProxyTunnel> MyCache;

  public:
    class Handler
    {
      public:
        Handler() {}

        virtual void onClosed(ProxyTunnel *tun) = 0;
        virtual void onError(ProxyTunnel *tun) = 0;
    };
    
    ProxyTunnel(EventPoller *poller)
            :mEventPoller(poller)
            ,mHandler(NULL)
            ,mLocalConn(poller)             
            ,mProxyConn(poller)
            ,mLocalCache(NULL)
            ,mProxyStatus(ProxyStatus_Closed)
    {
        memset(&mProxySvrAddr, 0, sizeof(mProxySvrAddr));
        *mDestSvrIp = '\0';
        mDestSvrPort = 0;
        *mHttpHeader = '\0';

        mLocalCache = new MyCache(this, &ProxyTunnel::onFlushLocal);
        assert(mLocalCache && "new local cache failed.");
    }

    virtual ~ProxyTunnel();

    // 从本地客户端接入连接
    bool acceptLocal(int connfd);

    // 隧道清理
    void cleanup();

    bool setProxyServer(const char *ip, int port);
    bool setDestServer(const char *ip, int port);

    void setHandler(Handler *h);

    virtual void onConnected(Connection *pConn);
    virtual void onDisconnected(Connection *pConn);

    virtual void onRecv(Connection *pConn, const void *data, size_t datalen);
    virtual void onError(Connection *pConn);

  private:
    void parseHttpHeader();

    // 将缓存的本地客户端发上来的数据发送到代理服务器
    void flushLocal();
    bool onFlushLocal(const void *data, size_t datalen);       

    void _onClose();
    void _onError();    

  private:
    EventPoller *mEventPoller;

    Handler *mHandler;

    Connection mLocalConn;
    Connection mProxyConn;

    MyCache *mLocalCache;

    sockaddr_in mProxySvrAddr; // 代理服务器地址
    char mDestSvrIp[IPv4_SIZE]; // 目标服务器IP
    int mDestSvrPort; // 目标服务器端口

    EProxyStatus mProxyStatus;
    char mHttpHeader[HTTP_HEADER_SIZE];
};

NAMESPACE_END // proxy

#endif // __PROXY_TUNNEL_H__