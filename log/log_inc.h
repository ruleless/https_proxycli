#ifndef __CILL_LOG_INC_H__
#define __CILL_LOG_INC_H__

/*
 * 平台定义
 */
#ifndef PLATFORM_WIN32
# define PLATFORM_WIN32 0
# define PLATFORM_UNIX  1
# define PLATFORM_APPLE 2
# define PLATFORM_INTEL 3
#endif /* #ifndef PLATFORM_WIN32 */

#ifndef UNIX_FLAVOUR_LINUX
# define UNIX_FLAVOUR_LINUX  1
# define UNIX_FLAVOUR_BSD    2
# define UNIX_FLAVOUR_OTHER  3
# define UNIX_FLAVOUR_OSX    4
#endif /* #ifndef UNIX_FLAVOUR_LINUX */

#ifndef PLATFORM
# if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#  define PLATFORM PLATFORM_WIN32
# elif defined( __INTEL_COMPILER )
#  define PLATFORM PLATFORM_INTEL
# elif defined( __APPLE_CC__ )
#  define PLATFORM PLATFORM_APPLE
# else
#  define PLATFORM PLATFORM_UNIX
# endif
#endif /* #ifndef PLATFORM */

/*
 * 线程定义
 */
#if PLATFORM == PLATFORM_WIN32
# ifndef THREAD_ID
#  define THREAD_ID                          HANDLE
#  define THREAD_SINGNAL                     HANDLE
#  define THREAD_SINGNAL_INIT(x)             x = CreateEvent(NULL, TRUE, FALSE, NULL)
#  define THREAD_SINGNAL_DELETE(x)           CloseHandle(x)
#  define THREAD_SINGNAL_SET(x)              SetEvent(x)
#  define THREAD_MUTEX                       CRITICAL_SECTION
#  define THREAD_MUTEX_INIT(x)               InitializeCriticalSection(&x)
#  define THREAD_MUTEX_DELETE(x)             DeleteCriticalSection(&x)
#  define THREAD_MUTEX_LOCK(x)               EnterCriticalSection(&x)
#  define THREAD_MUTEX_UNLOCK(x)             LeaveCriticalSection(&x)
# endif
#else
# ifndef THREAD_ID
#  define THREAD_ID                          pthread_t
#  define THREAD_SINGNAL                     pthread_cond_t
#  define THREAD_SINGNAL_INIT(x)             pthread_cond_init(&x, NULL)
#  define THREAD_SINGNAL_DELETE(x)           pthread_cond_destroy(&x)
#  define THREAD_SINGNAL_SET(x)              pthread_cond_signal(&x);
#  define THREAD_MUTEX                       pthread_mutex_t
#  define THREAD_MUTEX_INIT(x)               pthread_mutex_init (&x, NULL)
#  define THREAD_MUTEX_DELETE(x)             pthread_mutex_destroy(&x)
#  define THREAD_MUTEX_LOCK(x)               pthread_mutex_lock(&x)
#  define THREAD_MUTEX_UNLOCK(x)             pthread_mutex_unlock(&x)
# endif
#endif

/*
 * 日志打印宏定义
 */
#ifndef DISABLE_LOG
# define DebugPrint(fmt, ...)                                           \
    do { log_print(DebugLog, "[%s:%d][%s]" fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while (0)
# define InfoPrint(fmt, ...)     do { log_print(InfoLog, fmt, ##__VA_ARGS__); } while(0)
# define WarningPrint(fmt, ...)  do { log_print(WarningLog, fmt, ##__VA_ARGS__); } while(0)
# define ErrorPrint(fmt, ...)    do { log_print(ErrorLog, fmt, ##__VA_ARGS__); } while(0)
# define EmphasisPrint(fmt, ...) do { log_print(EmphasisLog, fmt, ##__VA_ARGS__); } while(0)
#else
# define DebugPrint(fmt, ...)
# define InfoPrint(fmt, ...)
# define WarningPrint(fmt, ...)
# define ErrorPrint(fmt, ...)
# define EmphasisPrint(fmt, ...)
#endif /* #ifndef DISABLE_LOG */

#define MAX_LOG_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 日志级别
 */
enum ELogLevel
{
    DebugLog      = 0x01,
    InfoLog       = 0x02,
    WarningLog    = 0x04,
    ErrorLog      = 0x08,
    EmphasisLog   = 0x10,

    AllLog = DebugLog|InfoLog|WarningLog|ErrorLog|EmphasisLog,
};

/*
 * 自定义的日志输出结构
 */
struct CustomLog
{
    void (*print_log)(ELogLevel level, time_t t, const char *time, const char *msg);
};

/*
 * 日志系统初始化
 */
int log_initialise(int level);

/*
 * 日志系统去初始化
 */
void log_finalise();

/*
 * 注册控制台Log
 */
void log_reg_console();

/*
 * 注册文件日志
 */
void log_reg_filelog(const char *suffix,
                     const char *curfile_prefix,
                     const char *curfile_dir,
                     const char *storefile_prefix,
                     const char *storefile_dir);

/*
 * 注册自定义的日志输出接口
 */
void log_reg_custom(CustomLog *log);

/*
 * 日志打印
 */
void log_print(int loglv, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __CILL_LOG_INC_H__ */
