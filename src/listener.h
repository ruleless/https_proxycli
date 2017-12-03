#ifndef __LISTENER_H__
#define __LISTENER_H__

#include "proxy_common.h"
#include "event_poller.h"

NAMESPACE_BEG(proxy)

class Listener : InputNotificationHandler
{
  public:
    struct Handler
    {
        virtual void onAccept(int connfd) = 0;
    };

    Listener(EventPoller *poller)
            :mFd(-1)
            ,mHandler(NULL)
            ,mEventPoller(poller)
    {
        assert(mEventPoller && "Listener::mEventPoller != NULL");
    }

    virtual ~Listener();

    bool initialise(const char *ip, int port);
    bool initialise(const sockaddr *sa, socklen_t salen);
    void finalise();

    inline void setEventHandler(Handler *h)
    {
        mHandler = h;
    }

    // InputNotificationHandler
    virtual int handleInputNotification(int fd);
  private:
    int mFd;
    Handler *mHandler;

    EventPoller *mEventPoller;
};

NAMESPACE_END // namespace proxy

#endif // __LISTENER_H__
