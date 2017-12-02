#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <iostream>
#include <list>

#include "log_inc.h"
#include "log.h"

#include "file_log.h"

NAMESPACE_BEG(core)

//--------------------------------------------------------------------------
// 输出到控制台
#ifndef CONSOLE_TEXT_BLACK
// font color
# define CONSOLE_TEXT_BLACK          0
# define CONSOLE_TEXT_RED            1
# define CONSOLE_TEXT_GREEN          2
# define CONSOLE_TEXT_YELLOW         3
# define CONSOLE_TEXT_BLUE           4
# define CONSOLE_TEXT_MAGENTA        5
# define CONSOLE_TEXT_CYAN           6
# define CONSOLE_TEXT_WHITE          7
# define CONSOLE_TEXT_BOLD           8
# define CONSOLE_TEXT_BOLD_RED       9
# define CONSOLE_TEXT_BOLD_GREEN     10
# define CONSOLE_TEXT_BOLD_YELLO     11
# define CONSOLE_TEXT_BOLD_BLUE      12
# define CONSOLE_TEXT_BOLD_MAGENTA   13
# define CONSOLE_TEXT_BOLD_CYAN      14
# define CONSOLE_TEXT_BOLD_WHITE     15

// background color
# define CONSOLE_BG_BLACK            0
# define CONSOLE_BG_RED              (1 << 4)
# define CONSOLE_BG_GREEN            (2 << 4)
# define CONSOLE_BG_YELLO            (3 << 4)
# define CONSOLE_BG_BLUE             (4 << 4)
# define CONSOLE_BG_MAGENTA          (5 << 4)
# define CONSOLE_BG_CYAN             (6 << 4)
# define CONSOLE_BG_WHITE            (7 << 4)
#endif

static void consoleSetColor(int color)
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD result = 0;
    if (color & 1) result |= FOREGROUND_RED;
    if (color & 2) result |= FOREGROUND_GREEN;
    if (color & 4) result |= FOREGROUND_BLUE;
    if (color & 8) result |= FOREGROUND_INTENSITY;
    if (color & 16) result |= BACKGROUND_RED;
    if (color & 32) result |= BACKGROUND_GREEN;
    if (color & 64) result |= BACKGROUND_BLUE;
    if (color & 128) result |= BACKGROUND_INTENSITY;
    SetConsoleTextAttribute(hConsole, (WORD)result);
#else
    int foreground = color & 7;
    int background = (color >> 4) & 7;
    int bold = color & 8;
    printf("\033[%s3%d;4%dm", bold? "01;" : "", foreground, background);
#endif
}

static void consoleResetColor()
{
#ifdef _WIN32
    consoleSetColor(7);
#else
    printf("\033[0m");
#endif
}

class ConsolePrinter : public ILogPrinter
{
  public:
    virtual void onPrint(ELogLevel level, time_t t, const char *time, const char *msg)
    {
        assert(msg != NULL);

        static int color[] =
        {
            0,
            CONSOLE_TEXT_WHITE,     // Debug
            CONSOLE_TEXT_GREEN,     // Info
            0,
            CONSOLE_TEXT_YELLOW,    // Warning
            0,0,0,
            CONSOLE_TEXT_RED,       // Error
            0,0,0,0,0,0,0,
            CONSOLE_TEXT_BOLD_RED,  // Emphasis
        };
        static const char *levelstr[] =
        {
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

        consoleSetColor(color[level]);

        if (time && hasTime())
        {
            printf("%s", time);
        }
        unsigned i = (unsigned)level;
        if (i < sizeof(levelstr) / sizeof(levelstr[0]) && levelstr[i])
            printf("%s", levelstr[i]);
        printf("%s\n", msg);
        consoleResetColor();
    }
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 输出到Html文件
class HtmlFilePrinter : public ILogPrinter
{
  public:
    HtmlFilePrinter() : mFile(0) {}

    ~HtmlFilePrinter()
    {
        if (mFile)
        {
            fputs("</font>\n</body>\n</html>", mFile);
            fclose(mFile);
        }
    }

    bool create(const char* filename, bool bWrite)
    {
        if(bWrite)
        {
            mFile = fopen(filename, "wt");
        }
        else
        {
            mFile = fopen(filename, "at");
        }

        if (!mFile)
        {
            return false;
        }
        assert(mFile != 0);

#ifdef _UNICODE
        uchar BOM[3] = {0xEF,0xBB,0xBF};
        fwrite(BOM, sizeof(BOM)/sizeof(uchar), 1, mFile);
        fputs(
            "<html>\n"
            "<head>\n"
            "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf8\">\n"
            "<title>Html Output</title>\n"
            "</head>\n"
            "<body>\n<font face=\"Fixedsys\" size=\"2\" color=\"#0000FF\">\n", mFile);
#else
        fputs(
            "<html>\n"
            "<head>\n"
            "<meta http-equiv=\"content-type\" content=\"text/html; charset=gb2312\">\n"
            "<title>Html Output</title>\n"
            "</head>\n"
            "<body>\n<font face=\"Fixedsys\" size=\"2\" color=\"#0000FF\">\n", mFile);
#endif
        return true;
    }

    virtual void onPrint(ELogLevel level, time_t t, const char *time, const char *msg)
    {
        assert(msg != NULL);

        static const char* color[] =
        {
            0,
            "<font color=\"#000000\">", // Info
            "<font color=\"#0000FF\">", // Trace
            0,
            "<font color=\"#FF00FF\">", // Warning
            0,0,0,
            "<font color=\"#FF0000\">", // Error
            0,0,0,0,0,0,0,
            "<font color=\"#FFFF00\" style =\"background-color:#6a3905;\">", // Emphasis
        };

        fputs(color[(int)level], mFile);

        if (time && hasTime())
        {
            fputs(time, mFile);
        }

        char* pStart = (char *)msg;
        char* pPos = pStart;
        char* pEnd = pStart + strlen(pStart) - sizeof(char);

        while (pPos <= pEnd)
        {
            if (*pPos == '\n') // 换行
            {
                if (pStart < pPos)
                {
                    fwrite(pStart, pPos - pStart, 1, mFile);
                }
                fputs("<br>", mFile);

                pStart = ++pPos;
            }
            else
            {
                pPos ++;
            }
        }

        if (pStart < pPos)
            fwrite(pStart, pPos - pStart, 1, mFile);

        fputs("</font>\n", mFile);

        fflush(mFile);
    }

  private:
    FILE* mFile;
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 文件日志
class FileLogImpl : public FileLog
{
  public:
    FileLogImpl() :FileLog()
    {
        *mCurFileDir = '\0';
        *mCurFilePrefix = '\0';
        *mStoreFileDir = '\0';
        *mStoreFilePrefix = '\0';
        *mSuffix = '\0';

        *mCurFileTmp = '\0';
        *mStoreFileTmp = '\0';
    }

    virtual ~FileLogImpl() {}

    void setCurFileInfo(const char *dir, const char *prefix)
    {
        snprintf(mCurFileDir, sizeof(mCurFileDir), "%s", dir);
        snprintf(mCurFilePrefix, sizeof(mCurFilePrefix), "%s", prefix);
    }

    void setStoreFileInfo(const char *dir, const char *prefix)
    {
        snprintf(mStoreFileDir, sizeof(mStoreFileDir), "%s", dir);
        snprintf(mStoreFilePrefix, sizeof(mStoreFilePrefix), "%s", prefix);
    }

    void setSuffix(const char *suffix)
    {
        snprintf(mSuffix, sizeof(mSuffix), "%s", suffix);
    }

  protected:
    virtual const char *_getTodayFilePath(const UtilDay &d)
    {
        snprintf(mCurFileTmp, sizeof(mCurFileTmp), "%s/%s%04d%02d%02d.%s",
                 mCurFileDir, mCurFilePrefix, d.year, d.month, d.day, mSuffix);
        return mCurFileTmp;
    }

    virtual const char *_getTodayFileDir() const
    {
        return mCurFileDir;
    }

    virtual const char *_getStoreFilePath(const UtilDay &d)
    {
        snprintf(mStoreFileTmp, sizeof(mStoreFileTmp), "%s/%s%04d%02d%02d.%s",
                 mStoreFileDir, mStoreFilePrefix, d.year, d.month, d.day, mSuffix);
        return mStoreFileTmp;
    }

    virtual const char *_getStoreFileDir() const
    {
        return mStoreFileDir;
    }

    virtual const char *_getSuffix() const
    {
        return mSuffix;
    }

    virtual const char *_todayFileDate(const char *filename) const
    {
        const char *datestr = strstr(filename, mCurFilePrefix);
        if (datestr == filename)
        {
            return datestr + strlen(mCurFilePrefix);
        }
        return NULL;
    }

    virtual const char *_storeFileDate(const char *filename) const
    {
        const char *datestr = strstr(filename, mStoreFilePrefix);
        if (datestr == filename)
        {
            return datestr + strlen(mStoreFilePrefix);
        }
        return NULL;
    }

  private:
    char mCurFileDir[PATH_MAX];
    char mCurFilePrefix[PATH_MAX];
    char mStoreFileDir[PATH_MAX];
    char mStoreFilePrefix[PATH_MAX];
    char mSuffix[PATH_MAX];

    char mCurFileTmp[PATH_MAX];
    char mStoreFileTmp[PATH_MAX];
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 自定义日志
class CustomLogPrinter : public ILogPrinter
{
    typedef std::list<CustomLog *> LogList;

  public:
    CustomLogPrinter() :ILogPrinter()
                    ,mLogList()
    {}

    virtual ~CustomLogPrinter() {}

    void regLog(CustomLog *log)
    {
        mLogList.push_back(log);
    }

    void unregLog(CustomLog *log)
    {
        for (LogList::iterator it = mLogList.begin(); it != mLogList.end(); it++)
        {
            if (*it == log)
            {
                mLogList.erase(it++);
                break;
            }
        }
    }

    virtual void onPrint(ELogLevel level, time_t t, const char *time, const char *msg)
    {
        for (LogList::iterator it = mLogList.begin(); it != mLogList.end(); it++)
        {
            CustomLog *l = *it;

            if (l)
            {
                l->print_log(level, t, time, msg);
            }
        }
    }

  private:
    LogList mLogList;
};
//--------------------------------------------------------------------------

static Log *gpLog = NULL;

static ConsolePrinter gConsolePrinter;
static HtmlFilePrinter gHtmlFilePrinter;
static FileLogImpl gFileLog;
static CustomLogPrinter gCustomLog;

NAMESPACE_END // namespace core

using namespace core;

extern "C" {

int log_initialise(int level)
{
    if (gpLog)
    {
        return 0;
    }

    gpLog = new Log();
    if (!gpLog)
    {
        return -1;
    }

    if (!gpLog->initialise(level, true))
    {
        return -1;
    }

    return 0;
}

void log_finalise()
{
    if (gpLog)
    {
        delete gpLog;
        gpLog = NULL;
    }
}

void log_reg_console()
{
    assert(gpLog && "log_reg_console && gpLog");

    gpLog->regPrinter(&gConsolePrinter);
}

void log_reg_filelog(const char *suffix,
                     const char *curfile_prefix,
                     const char *curfile_dir,
                     const char *storefile_prefix,
                     const char *storefile_dir)
{
    assert(gpLog && "log_reg_filelog && gpLog");

    if (!suffix || !curfile_prefix || !curfile_dir ||
        !storefile_prefix || !storefile_dir)
    {
        return;
    }

    gFileLog.setSuffix(suffix);
    gFileLog.setCurFileInfo(curfile_dir, curfile_prefix);
    gFileLog.setStoreFileInfo(storefile_dir, storefile_prefix);
    gFileLog.openRecentFile();

    gpLog->regPrinter(&gFileLog);
}

void log_reg_custom(CustomLog *log)
{
    assert(gpLog && "log_reg_custom && gpLog");

    if (!log)
    {
        return;
    }

    if (!gpLog->isRegitered(&gCustomLog))
    {
        gpLog->regPrinter(&gCustomLog);
    }

    gCustomLog.regLog(log);
}

void log_print(int loglv, const char *fmt, ...)
{
    assert(gpLog && "log_print && gpLog");

    va_list args;
    char buf[MAX_LOG_SIZE];

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    gpLog->printLog((ELogLevel)loglv, buf);
}

} // extern "C"
