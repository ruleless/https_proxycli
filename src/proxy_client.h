#ifndef __PROXY_CLIENT_H__
#define __PROXY_CLIENT_H__

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
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
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
typedef uint64_t                uint64;
typedef uint32_t                uint32;
typedef uint16_t                uint16;
typedef uint8_t                 uint8;
typedef uint16_t                WORD;
typedef uint32_t                DWORD;

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

#endif // __PROXY_CLIENT_H__
