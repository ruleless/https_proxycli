#include "proxy_tunnel.h"

NAMESPACE_BEG(proxy)

ProxyTunnel::~ProxyTunnel()
{
    delete mLocalCache;
}

bool ProxyTunnel::acceptLocal(int connfd)
{
    if (!mLocalConn.acceptConnection(connfd))
    {
        WarningPrint("[ProxyTunnel::acceptLocal] accept failed.");
        return false;
    }
    mLocalConn.setEventHandler(this);

    // 连接代理服务器
    mProxyStatus = ProxyStatus_Closed;
    mProxyConn.setEventHandler(this);
    if (!mProxyConn.connect((const sockaddr *)&mProxySvrAddr, (socklen_t)sizeof(mProxySvrAddr)))
    {
        mLocalConn.setEventHandler(NULL);
        mLocalConn.shutdown();
        mProxyConn.setEventHandler(NULL);
        WarningPrint("[ProxyTunnel::acceptLocal] connect proxy server error.");
        return false;
    }

    return true;
}

void ProxyTunnel::cleanup()
{
    mLocalCache->clear();
    mLocalConn.setEventHandler(NULL);
    mLocalConn.shutdown();

    mProxyStatus = ProxyStatus_Closed;
    mProxyConn.setEventHandler(NULL);
    mProxyConn.shutdown();
}

bool ProxyTunnel::setProxyServer(const char *ip, int port)
{
    mProxySvrAddr.sin_family = AF_INET;
    mProxySvrAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &mProxySvrAddr.sin_addr) < 0)
    {
        ErrorPrint("[ProxyTunnel::setDestServer] illegal ip(%s).", ip);
        return false;
    }

    return true;
}

bool ProxyTunnel::setDestServer(const char *ip, int port)
{
    struct sockaddr_in tmpaddr;

    if (inet_pton(AF_INET, ip, &tmpaddr.sin_addr) < 0)
    {
        ErrorPrint("[ProxyTunnel::setProxyServer] illegal ip(%s).", ip);
        return false;
    }

    snprintf(mDestSvrIp, sizeof(mDestSvrIp), "%s", ip);
    mDestSvrPort = port;

    return true;
}

void ProxyTunnel::setHandler(Handler *h)
{
    mHandler = h;
}

void ProxyTunnel::onConnected(Connection *pConn)
{
    if (pConn == &mProxyConn) // 与代理服务器连接成功
    {
        char header[HTTP_HEADER_SIZE] = {0};

        snprintf(header, sizeof(header),
                 HTTP_METHOD_CONNECT "\r\n"
                 HTTP_FIELD_HOST "\r\n"
                 "\r\n",
                 mDestSvrIp, mDestSvrPort,
                 mDestSvrIp, mDestSvrPort);

        mProxyStatus = ProxyStatus_Connecting;
        *mHttpHeader = '\0';
        mProxyConn.send(header, strlen(header));
    }
    else
    {
        assert(false && "[ProxyTunnel::onConnected] unexpected connction.");
    }
}

void ProxyTunnel::onDisconnected(Connection *pConn)
{
    if (pConn == &mLocalConn) // 与本地客户端连接断开
    {
        mLocalConn.shutdown();

        if (ProxyStatus_Connected == mProxyStatus)
        {
            flushLocal();
        }
        mProxyStatus = ProxyStatus_Closed;
        mProxyConn.shutdown();

        _onClose();
    }
    else if (pConn == &mProxyConn) // 与代理服务器连接断开
    {
        mProxyConn.shutdown();

        mLocalCache->clear();
        mLocalConn.shutdown();

        _onClose();
    }
    else
    {
        assert(false && "[ProxyTunnel::onDisconnected] unexpected connction.");
    }
}

void ProxyTunnel::onRecv(Connection *pConn, const void *data, size_t datalen)
{
    if (pConn == &mLocalConn) // 收到来自本地客户端的数据
    {
        if (ProxyStatus_Connected == mProxyStatus) // 已与代理建立连接
        {
            mProxyConn.send(data, datalen);
        }
        else
        {
            mLocalCache->cache(data, datalen); // 先缓存起来
        }
    }
    else if (pConn == &mProxyConn) // 收到来自代理服务器的数据
    {
        switch (mProxyStatus)
        {
        case ProxyStatus_Connecting: // 已向代理服务器发送CONNECT消息
            {
                char *ptr = mHttpHeader + strlen(mHttpHeader);
                char *endptr = mHttpHeader + sizeof(mHttpHeader) - 1;

                if (datalen > size_t(endptr - ptr))
                {
                    ErrorPrint("[ProxyTunnel::onRecv] buf overflow. datalen=%u", datalen);
                    mProxyConn.setEventHandler(NULL);
                    _onError();
                    return;
                }

                memcpy(ptr, data, datalen);
                ptr += datalen;
                *ptr = '\0';

                parseHttpHeader();
            }
            break;
        case ProxyStatus_Connected: // 已建立代理隧道
            {
                if (!mLocalConn.isConnected()) // 与本地客户端已断开连接
                {
                    ErrorPrint("[ProxyTunnel::onRecv] connection with local client is already closed.");
                    mProxyConn.setEventHandler(NULL);
                    _onError();
                    return;
                }

                mLocalConn.send(data, datalen);
            }
            break;
        default: // 代理隧道处于非法状态
            ErrorPrint("[ProxyTunnel::onRecv] illegal proxy status(%d).", mProxyStatus);
            mProxyConn.setEventHandler(NULL);
            _onError();
            break;
        }
    }
    else
    {
        assert(false && "[ProxyTunnel::onRecv] unexpected connction.");
    }
}

void ProxyTunnel::onError(Connection *pConn)
{
    if (pConn == &mLocalConn) // 本地连接出错
    {
        mLocalConn.shutdown();

        if (ProxyStatus_Connected == mProxyStatus)
        {
            flushLocal();
        }
        mProxyStatus = ProxyStatus_Closed;
        mProxyConn.setEventHandler(NULL);
        mProxyConn.shutdown();

        WarningPrint("connection with local occur error. reason:%s", strerror(errno));
        _onError();
    }
    else if (pConn == &mProxyConn) // 代理连接出错
    {
        mProxyConn.setEventHandler(NULL);
        mProxyConn.shutdown();

        mLocalCache->clear();
        mLocalConn.setEventHandler(NULL);
        mLocalConn.shutdown();

        WarningPrint("connection with proxy occur error. reason:%s", strerror(errno));
        _onError();
    }
    else
    {
        assert(false && "[ProxyTunnel::onError] unexpected connction.");
    }
}

void ProxyTunnel::parseHttpHeader()
{
    char *line_head = NULL, *line_tail = NULL;
    bool full_header = false;

    line_head = mHttpHeader;
    while ((line_tail = strstr(line_head, "\r\n")))
    {
        if (line_tail == line_head)
        {
            full_header = true;
            break;
        }

        line_head = line_tail + 2;
    }

    if (full_header)
    {
        if (strstrICase(mHttpHeader, mHttpHeader + strlen(mHttpHeader), SSL_CONNECTION_RESPONSE_OK))
        {
            mProxyStatus = ProxyStatus_Connected; // 代理隧道建立成功
            flushLocal(); // 先将缓存的本地客户端发送上来的数据发送出去
        }
        else
        {
            InfoPrint("build http tunnel failed. resp:%s", mHttpHeader);
            mProxyStatus = ProxyStatus_Error;
            mProxyConn.setEventHandler(NULL);
            _onError();
        }
    }
}

void ProxyTunnel::flushLocal()
{
    if (ProxyStatus_Connected == mProxyStatus)
    {
        mLocalCache->flushAll();
    }
}

bool ProxyTunnel::onFlushLocal(const void *data, size_t datalen)
{
    mProxyConn.send(data, datalen);
    return true;
}

void ProxyTunnel::_onClose()
{
    if (mHandler)
        mHandler->onClosed(this);
}

void ProxyTunnel::_onError()
{
    if (mHandler)
        mHandler->onError(this);
}

NAMESPACE_END // proxy
