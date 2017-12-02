#include <sys/statfs.h>

#include "file_log.h"

NAMESPACE_BEG(core)

FileLog::~FileLog()
{
    if (mLogFile)
    {
        fclose(mLogFile);
    }
}

void FileLog::onPrint(ELogLevel level, time_t t, const char *time, const char *msg)
{
    EState s = State_Start;
    UtilDay d(t);

    for (;;)
    {
        switch (s)
        {
        case State_Start:
            {
                if (!mLogFile)
                {
                    s = State_PreOpenCurFile;
                    break;
                }

                if (mCurDay != d)
                {
                    s = State_Dump;
                    break;
                }

                s = State_Ready;
            }
            break;
        case State_PreOpenCurFile:
            {
                assert(!mLogFile && "State_PreOpenCurFile:mLogFile < 0");

                const char *pathname = _getTodayFilePath(d);
                mCurDay = d;
                mLogFile = fopen(pathname, "at+");

                if (!mLogFile)
                {
                    s = State_Error;
                    break;
                }

                s = State_Ready;
            }
            break;
        case State_Dump:
            {
                assert(mLogFile && "State_Dump:mLogFile >= 0");

                // 如果磁盘空间不足则删除最早的日志文件
                while (checkDiskStatus() == DiskStatus_Full)
                {
                    if (_deleteEarliestFile() != ErrCode_Success) // 删除失败就不用继续了
                        break;
                }

                // 日志文件转储
                (void)_dump(mCurDay);

                // 不管转储成不成功，都需要保证当前的日志可以写入
                s = State_PreOpenCurFile;

                // 删除当前文件
                fclose(mLogFile);
                mLogFile = NULL;
                unlink(_getTodayFilePath(mCurDay));
                mCurDay.set((time_t)0);
            }
            break;
        case State_Ready:
            {
                assert(mLogFile && "State_Ready:mLogFile >= 0");

                int rv = _writeLog(level, t, time, msg);
                if (ErrCode_CurFileFull == rv)
                {
                    s = State_Dump;
                    break;
                }

                if (rv != ErrCode_Success)
                {
                    s = State_Error;
                    break;
                }

                s = State_Done;
            }
            break;
        default:
            break;
        }

        if (State_Error == s ||
            State_Done == s)
        {
            break;
        }
    }
}

FileLog::EErrCode FileLog::_writeLog(ELogLevel level, time_t t, const char *time, const char *msg)
{
    static const size_t FILE_SIZE = 5*1024*1024;
    static const char *LOG_LEVEL[] = {
        0,
        "[Debug]",     // Debug
        "[Info]",      // Info
        0,
        "[Warn]",      // Warning
        0,0,0,
        "[Error]",     // Error
        0,0,0,0,0,0,0,
        "[Emphasis]",  // Emphasis
    };

    EErrCode rv = ErrCode_Unknown;
    int n;
    unsigned i;
    int logfd = fileno(mLogFile);

    if (logfd < 0)
    {
        return ErrCode_Unknown;
    }

    // 文件过大则需要转储
    struct stat st;
    if (fstat(logfd, &st) != 0)
    {
        goto end;
    }

    if ((size_t)st.st_size >= FILE_SIZE)
    {
        rv = ErrCode_CurFileFull;
        goto end;
    }

    // 写日志
    i = (unsigned)level;
    if (i < sizeof(LOG_LEVEL) / sizeof(LOG_LEVEL[0]) && LOG_LEVEL[i])
    {
        if (time && hasTime())
            n = fprintf(mLogFile, "%s%s%s\n", time, LOG_LEVEL[i], msg);
        else
            n = fprintf(mLogFile, "%s%s\n", msg, LOG_LEVEL[i]);
    }
    else
    {
        return ErrCode_ArgError;
    }

    if (n < 0)
    {
        return ErrCode_WriteFail;
    }
    if (fflush(mLogFile))
    {
        return ErrCode_CurFileFull;
    }

    rv = ErrCode_Success;

end:
    return rv;
}

FileLog::EErrCode FileLog::_dump(const UtilDay &d)
{
    // 打开磁盘文件
    const char *storefile = _getStoreFilePath(d);
    int storefd = open(storefile, O_RDWR|O_CREAT|O_APPEND, 0664);
    EErrCode rv = ErrCode_Success;

    char buf[BLK_SIZE];
    int count = 0, n = 0;

    if (storefd < 0)
    {
        rv = ErrCode_Unknown;
        goto end;
    }

    // 当前文件的读指针移到最前
    if (fseek(mLogFile, 0, SEEK_SET) < 0)
    {
        rv = ErrCode_Unknown;
        goto end;
    }

    // 将当前文件转储
    count = 0, n = 0;

    while ((n = fread(buf, 1, sizeof(buf), mLogFile)) > 0)
    {
  again:
        if (write(storefd, buf, n) < 0)
        {
            if (EINTR == errno)
            {
                goto again;
            }
            else if (ENOSPC == errno)
            {
                rv = ErrCode_DiskFull;
            }
            else
            {
                rv = ErrCode_WriteFail;
            }
            break;
        }
    }

end:
    if (storefd >= 0)
    {
        close(storefd);
    }

    return rv;
}

FileLog::EErrCode FileLog::_deleteExpireFile(int expireDay)
{
    DIR *dir = opendir(_getStoreFileDir());
    char pathname[PATH_MAX];

    if (!dir)
    {
        return ErrCode_Unknown;
    }

    UtilDay today(time(NULL)), d;
    bool deleted = false;
    const char *datestr = NULL;
    struct dirent *ent = readdir(dir);
    while (ent != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
            goto read_next;
        }

        if (!strstr(ent->d_name, _getSuffix()))
        {
            goto read_next;
        }

        if (strlen(ent->d_name) <= strlen("20170201"))
        {
            goto read_next;
        }

        if (!(datestr = _storeFileDate(ent->d_name)))
        {
            goto read_next;
        }

        if (!d.parse(datestr))
        {
            goto read_next;
        }

        if (today - d > expireDay)
            /* 超过期限，可以删除 */
        {
            snprintf(pathname, sizeof(pathname), "%s/%s", _getStoreFileDir(), ent->d_name);
            if (access(pathname, F_OK) == 0)
            {
                deleted = true;
                unlink(pathname);
            }
        }

  read_next:
        ent = readdir(dir);
    }

    closedir(dir);

    if (!deleted)
    {
        return ErrCode_NoExpireFile;
    }

    return ErrCode_Success;
}

FileLog::EErrCode FileLog::_deleteEarliestFile()
{
    DIR *dir = opendir(_getStoreFileDir());
    if (!dir)
    {
        return ErrCode_Unknown;
    }

    UtilDay today(time(NULL)), targetDay(time_t(0)), d;
    int maxDiffDay = 0;
    const char *datestr = NULL;
    struct dirent *ent = readdir(dir);
    while (ent != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
            goto read_next;
        }

        if (!strstr(ent->d_name, _getSuffix()))
        {
            goto read_next;
        }

        if (strlen(ent->d_name) <= strlen("20170201"))
        {
            goto read_next;
        }

        if (!(datestr = _storeFileDate(datestr)))
        {
            goto read_next;
        }

        if (!d.parse(datestr))
        {
            goto read_next;
        }

        if (today - d > maxDiffDay)
        {
            maxDiffDay = today - d;
            targetDay  = d;
        }

  read_next:
        ent = readdir(dir);
    }

    bool deleted = false;
    if (targetDay.valid())
    {
        const char *pathname = _getStoreFilePath(targetDay);
        if (access(pathname, F_OK) == 0)
        {
            deleted = true;
            unlink(pathname);
        }
    }

    closedir(dir);

    if (!deleted)
    {
        return ErrCode_NoExpireFile;
    }

    return ErrCode_Success;
}

void FileLog::openRecentFile()
{
    const char *curdir = _getTodayFileDir();
    DIR *dir = opendir(curdir);

    if (!dir)
    {
        return;
    }

    UtilDay today(time(NULL)), d, latest_day;
    int minDiff = 999999;
    char pathname[PATH_MAX], target[PATH_MAX] = {0};
    FList flist;

    const char *datestr = NULL;
    struct dirent *ent = readdir(dir);
    while (ent != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
            goto read_next;
        }

        if (!strstr(ent->d_name, _getSuffix()))
        {
            goto read_next;
        }

        if (strlen(ent->d_name) <= strlen("20170201"))
        {
            goto read_next;
        }

        if (!(datestr = _todayFileDate(ent->d_name)))
        {
            goto read_next;
        }

        if (!d.parse(datestr))
        {
            goto read_next;
        }

        snprintf(pathname, sizeof(pathname), "%s/%s", curdir, ent->d_name);
        if (access(pathname, F_OK) == 0)
        {
            int n = today - d;
            if (n < minDiff)
            {
                minDiff = n;
                latest_day = d;
                strncpy(target, pathname, sizeof(target) - 1);
            }
            flist.push_back(pathname);
        }

  read_next:
        ent = readdir(dir);
    }

    closedir(dir);

    for (FList::iterator it = flist.begin(); it != flist.end(); it++)
    {
        if (strcmp((*it).c_str(), target))
        {
            unlink((*it).c_str());
        }
    }
    flist.clear();

    if (latest_day.valid())
    {
        if ((mLogFile = fopen(pathname, "at+")))
        {
            mCurDay = latest_day;
        }
    }
}

unsigned long long FileLog::getDiskSpace() const
{
    const char *dir = _getStoreFileDir();
    struct statfs diskInfo;

    if (statfs(dir, &diskInfo) == 0)
    	return (unsigned long long)diskInfo.f_bsize * diskInfo.f_bavail;

    return 0;
}

FileLog::EDiskStatus FileLog::checkDiskStatus()
{
    unsigned long long leftsize = 0, alarmsize = 0;
    EDiskStatus status = DiskStatus_Idle;

    leftsize = getDiskSpace();
    alarmsize = getDiskAlarmSize();

    if (leftsize < alarmsize)
    {
        status = DiskStatus_Full;
    }

    return status;
}

NAMESPACE_END
