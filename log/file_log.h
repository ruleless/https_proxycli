#ifndef __CILL_FILE_LOG_H__
#define __CILL_FILE_LOG_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <list>
#include <string>

#include "log.h"

NAMESPACE_BEG(core)

class UtilDay
{
  public:
    UtilDay() : year(0), month(0), day(0) {}

    UtilDay(time_t t)
    {
        set(t);
    }

    ~UtilDay() {}

    void set(time_t t)
    {
        struct tm ltm;

		localtime_r(&t, &ltm);
		year  = ltm.tm_year + 1900;
		month = ltm.tm_mon + 1;
        day  = ltm.tm_mday;
    }

    bool parse(const char *s)
    {
#define _ASSIGN                                 \
        do {                                    \
            k = *ptr++ - '0';                   \
            if (k < 0 || k > 9)                 \
                goto fail;                      \
        } while(0)

        const char *ptr = s;
        int n, i, k;

        year = 0;
        n = 1000;
        for (i = 0; i < 4; i++)
        {
            _ASSIGN;
            year += k * n;
            n /= 10;
        }

        month = 0;
        n = 10;
        for (i = 0; i < 2; i++)
        {
            _ASSIGN;
            month += k * n;
            n /= 10;
        }
        if (month > 12)
            goto fail;

        day = 0;
        n = 10;
        for (i = 0; i < 2; i++)
        {
            _ASSIGN;
            day += k * n;
            n /= 10;
        }
        if (day > 31)
            goto fail;

        return true;

  fail:
        year = month = day = 0;
        return false;

#undef _ASSIGN
    }

    bool valid() const
    {
        return year != 0 || month != 0 || day != 0;
    }

    int operator-(const UtilDay &d) const
    {
        time_t t1, t2;
        struct tm tm1, tm2;

        memset(&tm1, 0, sizeof(tm1));
        memset(&tm2, 0, sizeof(tm2));

        tm1.tm_year = year - 1900;
        tm1.tm_mon  = month - 1;
        tm1.tm_mday = day;
        tm2.tm_year = d.year - 1900;
        tm2.tm_mon  = d.month - 1;
        tm2.tm_mday = d.day;

        t1 = mktime(&tm1);
        t2 = mktime(&tm2);
        long long diff = (long long)t1 - (long long)t2;
        return (int)(diff / (24*3600));
    }

    bool operator==(const UtilDay &d) const
    {
        return year == d.year && month == d.month && day == d.day;
    }

    bool operator!=(const UtilDay &d) const
    {
        return !this->operator==(d);
    }

  public:
    int year, month, day;
};

class FileLog : public ILogPrinter
{
    enum EState
    {
        State_Start,
        State_Error,
        State_Done,

        State_PreOpenCurFile,
        State_Dump,
        State_Ready,
    };

    enum EErrCode
    {
        ErrCode_Success,
        ErrCode_Unknown,

        ErrCode_DataErr,
        ErrCode_WriteHeaderFail,
        ErrCode_DiskFull,
        ErrCode_CurFileFull,
        ErrCode_WriteFail,
        ErrCode_FileExist,
        ErrCode_ArgError,

        ErrCode_NoExpireFile,
    };

    enum EDiskStatus
    {
        DiskStatus_Idle,
        DiskStatus_Full,
    };

  public:
    FileLog() :ILogPrinter()
              ,mLogFile(NULL)
    {}

    virtual ~FileLog();

    virtual void onPrint(ELogLevel level, time_t t, const char *time, const char *msg);

    EErrCode _writeLog(ELogLevel level, time_t t, const char *time, const char *msg);
    EErrCode _dump(const UtilDay &d);
    EErrCode _deleteExpireFile(int expireDay);
    EErrCode _deleteEarliestFile();

    void openRecentFile();

    // 获取磁盘大小
    unsigned long long getDiskSpace() const;

    // 获取磁盘告警时的大小
    virtual unsigned long long getDiskAlarmSize() const
    {
        return 1<<30;
    }

    // 检查磁盘状态
    EDiskStatus checkDiskStatus();

  protected:
    virtual const char *_getTodayFilePath(const UtilDay &d) = 0;
    virtual const char *_getTodayFileDir() const = 0;
    virtual const char *_getStoreFilePath(const UtilDay &d) = 0;
    virtual const char *_getStoreFileDir() const = 0;

    virtual const char *_getSuffix() const = 0;
    virtual const char *_todayFileDate(const char *filename) const = 0;
    virtual const char *_storeFileDate(const char *filename) const = 0;

  protected:
    typedef std::list<std::string> FList;
    static const size_t BLK_SIZE = 4096;

    FILE *mLogFile;
    UtilDay mCurDay;
};

NAMESPACE_END

#endif // __CILL_FILE_LOG_H__
