#include "proxy_tunnel.h"

NAMESPACE_BEG(proxy)

ProxyTunnel::~ProxyTunnel()
{}

bool ProxyTunnel::acceptLocal(int connfd)
{}

bool ProxyTunnel::setProxyServer(const char *ip, int port)
{}

bool ProxyTunnel::setDestServer(const char *ip, int port)
{}

void ProxyTunnel::onConnected(Connection *pConn)
{}

void ProxyTunnel::onDisconnected(Connection *pConn)
{}

void ProxyTunnel::onRecv(Connection *pConn, const void *data, size_t datalen)
{}

void ProxyTunnel::onError(Connection *pConn)
{}

bool connectProxy();

NAMESPACE_END // proxy
