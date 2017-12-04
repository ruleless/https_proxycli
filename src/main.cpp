#include "proxy_common.h"
#include "proxy_client.h"

using namespace proxy;

void sigHandler(int signo)
{
    switch (signo)
    {
    case SIGPIPE:
        {
            WarningPrint("broken pipe!");
        }
        break;
    case SIGINT:
        {
            InfoPrint("catch SIGINT!");
            gProxyClient.exitLoop();
        }
        break;
    case SIGQUIT:
        {
            InfoPrint("catch SIGQUIT!");
            gProxyClient.exitLoop();
        }
        break;
    case SIGKILL:
        {
            InfoPrint("catch SIGKILL!");
            gProxyClient.exitLoop();
        }
        break;
    case SIGTERM:
        {
            InfoPrint("catch SIGTERM!");
            gProxyClient.exitLoop();
        }
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    int opt = 0;
    const char *bindaddr = NULL, *destaddr = NULL, *proxyaddr = NULL;
    std::vector<std::string> vbindaddr, vdestaddr, vproxyaddr;

    while ((opt = getopt(argc, argv, "l:t:r:")) != -1)
    {
        switch (opt)
        {
        case 'l':
            bindaddr = optarg;
            break;
        case 't':
            destaddr = optarg;
            break;
        case 'r':
            proxyaddr = optarg;
        default:
            break;
        }
    }

    if (!bindaddr)
    {
        fprintf(stderr, "no listen address\n");
        exit(1);
    }
    if (!proxyaddr)
    {
        fprintf(stderr, "no proxy server\n");
        exit(1);
    }
    if (!destaddr)
    {
        fprintf(stderr, "no target server\n");
        exit(1);
    }

    split(std::string(bindaddr), ':', vbindaddr);
    split(std::string(proxyaddr), ':', vproxyaddr);
    split(std::string(destaddr), ':', vdestaddr);

    if (vbindaddr.size() != 2 ||
        vproxyaddr.size() != 2 ||
        vdestaddr.size() != 2)
    {
        fprintf(stderr, "ip format error!\n");
        exit(1);
    }

    log_initialise(AllLog);
    log_reg_console();
    log_reg_filelog("log", "http-proxy-", "/tmp", "http-proxy-old-", "/tmp");

    if (!gProxyClient.initialise(vbindaddr[0].c_str(), atoi(vbindaddr[1].c_str())))
    {
        log_finalise();
        exit(1);
    }
    if (!gProxyClient.setProxyServer(vproxyaddr[0].c_str(), atoi(vproxyaddr[1].c_str())))
    {
        gProxyClient.finalise();
        log_finalise();
        exit(1);
    }
    if (!gProxyClient.setDestServer(vdestaddr[0].c_str(), atoi(vdestaddr[1].c_str())))
    {
        gProxyClient.finalise();
        log_finalise();
        exit(1);
    }

    // set signal
    struct sigaction newAct;
    newAct.sa_handler = sigHandler;
    sigemptyset(&newAct.sa_mask);
    newAct.sa_flags = 0;

    sigaction(SIGPIPE, &newAct, NULL);

    sigaction(SIGINT, &newAct, NULL);
    sigaction(SIGQUIT, &newAct, NULL);

    // sigaction(SIGKILL, &newAct, NULL);
    sigaction(SIGTERM, &newAct, NULL);

    gProxyClient.runLoop();

    log_finalise();

    exit(0);
}
