#ifndef __PROXY_COMMON_H__
#define __PROXY_COMMON_H__

//--------------------------------------------------------------------------
// common include
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <deque>
#include <queue>
#include <limits>
#include <algorithm>
#include <utility>
#include <functional>
#include <cctype>
#include <iterator>

// unix include
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <signal.h>
#include <dirent.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 操作系统相关宏定义
#ifndef unix
# if __APPLE__
#  include "TargetConditionals.h"
#  if TARGET_IPHONE_SIMULATOR
#  elif TARGET_OS_IPHONE
#  elif TARGET_OS_MAC
#   define unix
#  else
#   error "Unknown Apple platform"
#  endif
# elif __ANDROID__
# elif __linux__
#  define unix
# elif __unix__
#  define unix
# elif defined(_POSIX_VERSION)
# else
#  error "Unknown compiler"
# endif
#endif

// 编译器定义
#define COMPILER_MICROSOFT 0
#define COMPILER_GNU       1
#define COMPILER_BORLAND   2
#define COMPILER_INTEL     3
#define COMPILER_CLANG     4

#ifdef _MSC_VER
# define COMPILER COMPILER_MICROSOFT
#elif defined( __INTEL_COMPILER )
# define COMPILER COMPILER_INTEL
#elif defined( __BORLANDC__ )
# define COMPILER COMPILER_BORLAND
#elif defined( __GNUC__ )
# define COMPILER COMPILER_GNU
#elif defined( __clang__ )
# define COMPILER COMPILER_CLANG
#else
# pragma error "FATAL ERROR: Unknown compiler."
#endif
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 常用宏定义
#define NAMESPACE_BEG(spaceName) namespace spaceName {
#define NAMESPACE_END }

#ifndef max
# define max(a, b) ((b) > (a) ? (b) : (a))
#endif
#ifndef min
# define min(a, b) ((b) < (a) ? (b) : (a))
#endif

#define LISTENQ 32
#define IPv4_SIZE sizeof("255.255.255.255")
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 类型定义
NAMESPACE_BEG(proxy)

typedef char                    mchar;
typedef wchar_t                 wchar;
typedef std::string             mstring;
typedef std::wstring            wstring;

#ifdef _UNICODE
typedef wchar                   tchar;
typedef wstring                 tstring;
#else
typedef mchar                   tchar;
typedef mstring                 tstring;
#endif

typedef unsigned char           uchar;
typedef unsigned short          ushort;
typedef unsigned int            uint;
typedef unsigned long           ulong;

#if COMPILER != COMPILER_GNU
typedef signed __int64          int64;
typedef signed __int32          int32;
typedef signed __int16          int16;
typedef signed __int8           int8;
typedef unsigned __int64        uint64;
typedef unsigned __int32        uint32;
typedef unsigned __int16        uint16;
typedef unsigned __int8         uint8;
typedef INT_PTR                 intptr;
typedef UINT_PTR                uintptr;
#else // #if COMPILER != COMPILER_GNU
typedef int64_t                 int64;
typedef int32_t                 int32;
typedef int16_t                 int16;
typedef int8_t                  int8;
typedef unsigned long long      uint64;
typedef unsigned int            uint32;
typedef unsigned short          uint16;
typedef unsigned char           uint8;

#ifdef _LP64
typedef int64                   intptr;
typedef uint64                  uintptr;
#else // #ifdef LP64
typedef int32                   intptr;
typedef uint32                  uintptr;
#endif // #ifdef LP64

#endif // #if COMPILER != COMPILER_GNU

NAMESPACE_END // namespace proxy
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 日志定义
#ifdef _USE_KLOG
# define DEBUG DebugPrint
#include "log/log_inc.h"
#else
# define DebugPrint(fmt, ...)
# define InfoPrint(fmt, ...)
# define WarningPrint(fmt, ...)
# define ErrorPrint(fmt, ...)
# define EmphasisPrint(fmt, ...)
# define DEBUG(fmt, ...)
#endif
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 套接字状态
enum EReason
{
    Reason_Success             = 0,

    Reason_TimerExpired        = -1,
    Reason_NoSuchPort          = -2,
    Reason_GeneralNetwork      = -3,
    Reason_CorruptedPacket     = -4,
    Reason_NonExistentEntry    = -5,
    Reason_WindowOverflow      = -6,
    Reason_Inactivity          = -7,
    Reason_ResourceUnavailable = -8,
    Reason_ClientDisconnected  = -9,
    Reason_TransmitQueueFull   = -10,
    Reason_ShuttingDown        = -11,
    Reason_WebSocketError      = -12,
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
struct Packet
{
    char *buf;
    size_t buflen;

    Packet() : buf(NULL), buflen(0)
    {
    }

    virtual ~Packet()
    {
        if (buf)
            free(buf);
    }
};

struct TcpPacket : public Packet
{
    size_t sentlen;

    TcpPacket() : Packet(), sentlen(0)
    {
    }

    virtual ~TcpPacket()
    {
    }
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// 通用函数定义

NAMESPACE_BEG(proxy)

/*
 * 设置套接字非阻塞
 * return true 设置成功 false 设置失败
 */
bool setNonblocking(int fd);

/*
 * 获取64位计算机时钟
 */
uint64 getClock64();

/*
 * 获取32位计算机时钟
 */
uint32 getClock();

/*
 * 字串查找(忽略大小写)
 */
char *strstrICase(const char *strStart, const char *strEnd, const char *substr);

/*
 * 是否是合法的ip
 */
bool isValidIp(const char *ip);
bool isValidPort(int port);

/*
 * 字符串分割
 */
template<typename T>
inline void split(const std::basic_string< T >& s, T c, std::vector< std::basic_string< T > > &v)
{
    if(s.size() == 0)
        return;

    typename std::basic_string< T >::size_type i = 0;
    typename std::basic_string< T >::size_type j = s.find(c);

    while(j != std::basic_string<T>::npos)
    {
        std::basic_string<T> buf = s.substr(i, j - i);

        if(buf.size() > 0)
            v.push_back(buf);

        i = ++j; j = s.find(c, j);
    }

    if(j == std::basic_string<T>::npos)
    {
        std::basic_string<T> buf = s.substr(i, s.length() - i);
        if(buf.size() > 0)
            v.push_back(buf);
    }
}

NAMESPACE_END // proxy
//--------------------------------------------------------------------------

#endif // __PROXY_COMMON_H__
