#ifndef __SELECT_POLLER_H__
#define __SELECT_POLLER_H__

#include "event_poller.h"

NAMESPACE_BEG(proxy)

class SelectPoller : public EventPoller
{
  public:
    SelectPoller();
  protected:
    virtual bool doRegisterForRead(int fd);
    virtual bool doRegisterForWrite(int fd);

    virtual bool doDeregisterForRead(int fd);
    virtual bool doDeregisterForWrite(int fd);

    virtual int processPendingEvents(double maxWait);
  private:
    void handleNotifications(int &countReady, fd_set &readFDs, fd_set &writeFDs);

    fd_set mFdReadSet;
    fd_set mFdWriteSet;

    int mMaxFd;
    int mFdWriteCount;
};

NAMESPACE_END // namespace proxy

#endif // __SELECT_POLLER_H__
