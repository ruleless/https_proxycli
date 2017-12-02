#ifndef __CILL_LOG_H__
#define __CILL_LOG_H__

#include <iostream>
#include <string>
#include <list>

#include "log_inc.h"

#if PLATFORM == PLATFORM_WIN32
# include <windows.h>
# include <process.h>
#endif

#ifndef NAMESPACE_BEG
# define NAMESPACE_BEG(spaceName) namespace spaceName {
# define NAMESPACE_END }
#endif

// 使用kmem做内存管理
#ifdef _USE_KMEM
# include "kmem/imembase.h"

# define malloc  ikmem_malloc
# define realloc ikmem_realloc
# define free    ikmem_free

# if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
inline void *__cdecl operator new(size_t size)   { return malloc(size); }
inline void *__cdecl operator new[](size_t size) { return malloc(size); }
inline void __cdecl operator delete(void* ptr)   { free(ptr); }
inline void __cdecl operator delete[](void* ptr) { free(ptr); }
# else
inline void *operator new(size_t size) throw(std::bad_alloc)   { return malloc(size); }
inline void *operator new[](size_t size) throw(std::bad_alloc) { return malloc(size); }
inline void operator delete(void* ptr) throw()   { free(ptr); }
inline void operator delete[](void* ptr) throw() { free(ptr); }
# endif
#endif

NAMESPACE_BEG(core)

//--------------------------------------------------------------------------
// 日志输出需继承该口
class ILogPrinter
{
  public:
    ILogPrinter() : mLevel(AllLog), mHasTime(true) {}

    virtual ~ILogPrinter() {}

    bool hasTime() const
    {
        return mHasTime;
    }

    void hasTime(bool b)
    {
        mHasTime = b;
    }

    void setLevel(int lvl)
    {
        mLevel = lvl;
    }

    int getLevel() const
    {
        return mLevel;
    }

    virtual void onPrint(ELogLevel level, time_t t, const char *time, const char *msg) {}

  protected:
    int mLevel;
    bool mHasTime;
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Log
class Log
{
  public:
    Log();
    ~Log();

    bool initialise(int level, bool hasTime);
    void finalise();

    void _flushOutlist();

    int getLogLevel() const;
    int setLogLevel(int level);

    bool hasTime(bool b);

    void regPrinter(ILogPrinter *p);
    void unregPrinter(ILogPrinter *p);
    bool isRegitered(ILogPrinter *p);

    void printLog(ELogLevel level, const char *msg);

#if PLATFORM == PLATFORM_WIN32
    static unsigned __stdcall _logProc(void *arg);
#else
    static void* _logProc(void* arg);
#endif

  protected:
    struct _MSG
    {
        std::string msg;
        time_t time;
        ELogLevel level;
    };

    typedef std::list<_MSG> MsgList;
    typedef std::list<ILogPrinter *> PrinterList;

    PrinterList mLogPrinter;
    int mLevel;
    bool mHasTime;

    volatile bool mbExit;
    bool mInited;
    THREAD_ID mTid;
    THREAD_MUTEX mMutex;
    THREAD_SINGNAL mCond;
    MsgList mMsgs1;
    MsgList mMsgs2;
    MsgList *mInlist;
    MsgList *mOutlist;
};
//--------------------------------------------------------------------------

NAMESPACE_END // namespace core

#endif // __CILL_LOG_H__
