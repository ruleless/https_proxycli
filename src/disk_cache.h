#ifndef __DISK_CACHE_H__
#define __DISK_CACHE_H__

#include "proxy_common.h"

NAMESPACE_BEG(proxy)

class DiskCache
{
  public:
    DiskCache()
            :mpFile(NULL)
    {}

    virtual ~DiskCache();

    ssize_t write(const void *data, size_t datalen);
    ssize_t read(void *data, size_t datalen);
    size_t peeksize();

    void rollback(size_t n);

    void clear();

  private:
    bool _createFile();

  private:
    FILE *mpFile;
};

NAMESPACE_END // namespace proxy

#endif // __DISKCACHE_H__
