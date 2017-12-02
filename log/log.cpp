#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

NAMESPACE_BEG(core)

Log::Log()
        :mLogPrinter()
        ,mLevel(AllLog)
        ,mHasTime(true)
        ,mbExit(false)
        ,mInited(false)
        ,mMsgs1()
        ,mMsgs2()
        ,mInlist(NULL)
        ,mOutlist(NULL)
{
}

Log::~Log()
{
    finalise();
}

bool Log::initialise(int level, bool hasTime)
{
    if (mInited)
        return true;

    mLevel = level;
    mHasTime = hasTime;
    mInlist = &mMsgs1;
    mOutlist = &mMsgs2;

#if PLATFORM == PLATFORM_WIN32
    if ((mTid = (THREAD_ID)_beginthreadex(NULL, 0, &Log::_logProc, (void *)this, NULL, 0)) == NULL)
    {
        return false;
    }

    InitializeCriticalSection(&mMutex);
    mCond = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == mCond)
    {
        return false;
    }
#else
    if(pthread_create(&mTid, NULL, Log::_logProc, (void *)this) != 0)
    {
        return false;
    }

    if (pthread_mutex_init(&mMutex, NULL) != 0)
    {
        return false;
    }
    if (pthread_cond_init(&mCond, NULL) != 0)
    {
        return false;
    }
#endif
    mInited = true;
    return true;
}

void Log::finalise()
{
    if (!mInited)
        return;

    mbExit = true;
    THREAD_SINGNAL_SET(mCond);
#if PLATFORM == PLATFORM_WIN32
    ::WaitForSingleObject(mTid, INFINITE);
    ::CloseHandle(mTid);
#else
    pthread_join(mTid, NULL);
#endif

    mOutlist = mInlist;
    _flushOutlist();

    THREAD_SINGNAL_DELETE(mCond);
    THREAD_MUTEX_DELETE(mMutex);

    mLogPrinter.clear();

    mInited = false;
}

#if PLATFORM == PLATFORM_WIN32
unsigned __stdcall Log::_logProc(void *arg)
#else
        void* Log::_logProc(void* arg)
#endif
{
    Log *pLog = (Log *)arg;
    while (!pLog->mbExit)
    {
        THREAD_MUTEX_LOCK(pLog->mMutex);
        while (pLog->mInlist->empty() && !pLog->mbExit)
        {
#if PLATFORM == PLATFORM_WIN32
            THREAD_MUTEX_UNLOCK(pLog->mMutex);
            ::WaitForSingleObject(pLog->mCond, INFINITE);
            THREAD_MUTEX_LOCK(pLog->mMutex);
#else
            pthread_cond_wait(&pLog->mCond, &pLog->mMutex);
#endif
        }

        Log::MsgList *tmpList = pLog->mOutlist;
        pLog->mOutlist = pLog->mInlist;
        pLog->mInlist = tmpList;
        THREAD_MUTEX_UNLOCK(pLog->mMutex);

        pLog->_flushOutlist();
    }

    return 0;
}

void Log::_flushOutlist()
{
    MsgList *outlist = mOutlist;
    if (!outlist->empty())
    {
        char timeStr[32] = {0};
        const char *pTimeStr = 0;
        for (MsgList::iterator i = outlist->begin(); i != outlist->end(); ++i)
        {
            _MSG &msgnode = *i;

            if (msgnode.time)
            {
                struct tm *timeinfo = localtime(&msgnode.time);
                snprintf(timeStr, sizeof(timeStr), "[%04d/%02d/%d %02d:%02d:%02d]",
                         timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                pTimeStr = timeStr;
            }
            else
            {
                pTimeStr = 0;
            }

            for (PrinterList::iterator it = mLogPrinter.begin(); it != mLogPrinter.end(); ++it)
            {
                ILogPrinter *obj = *it;
                if (obj->getLevel() & msgnode.level)
                {
                    obj->onPrint(msgnode.level, msgnode.time, pTimeStr, msgnode.msg.c_str());
                }
            }
        }

        outlist->clear();
    }
}

int Log::getLogLevel() const
{
    return mLevel;
}

int Log::setLogLevel(int level)
{
    THREAD_MUTEX_LOCK(mMutex);
    int old = mLevel;
    mLevel = level;
    THREAD_MUTEX_UNLOCK(mMutex);

    return old;
}

bool Log::hasTime(bool b)
{
    THREAD_MUTEX_LOCK(mMutex);
    bool old = mHasTime;
    mHasTime = b;
    THREAD_MUTEX_UNLOCK(mMutex);

    return old;
}

void Log::regPrinter(ILogPrinter *p)
{
    if (NULL == p)
        return;

    mLogPrinter.remove(p);
    mLogPrinter.push_back(p);
}

void Log::unregPrinter(ILogPrinter *p)
{
    if (NULL == p)
        return;

    for (PrinterList::iterator it = mLogPrinter.begin(); it != mLogPrinter.end(); ++it)
    {
        if (*it == p)
        {
            mLogPrinter.erase(it);
            break;
        }
    }
}

bool Log::isRegitered(ILogPrinter *p)
{
    for (PrinterList::iterator it = mLogPrinter.begin(); it != mLogPrinter.end(); ++it)
    {
        if (*it == p)
        {
            return true;
        }
    }
    return false;
}

void Log::printLog(ELogLevel level, const char *msg)
{
    if (NULL == msg)
        return;

    if ((mLevel & (int)level) == 0)
        return;

#if PLATFORM == PLATFORM_WIN32
# ifdef _DEBUG
    if (::IsDebuggerPresent())
    {
        ::OutputDebugString(msg);
    }
# endif
#endif

    THREAD_MUTEX_LOCK(mMutex);

    mInlist->push_back(_MSG());
    _MSG &msgnode = mInlist->back();
    msgnode.level = level;
    msgnode.time = mHasTime ? time(0) : 0;
    msgnode.msg = msg;

    THREAD_MUTEX_UNLOCK(mMutex);

    THREAD_SINGNAL_SET(mCond);
}

NAMESPACE_END // namespace core
