/***************************************************************************
 *  Windows Compatibility Header for Dystopia MUD                          *
 *  Provides POSIX compatibility layer for Windows builds                  *
 ***************************************************************************/

#ifndef COMPAT_H
#define COMPAT_H

#ifdef WIN32

/* ============================================
 * SECTION 1: Windows Headers
 * Must include winsock2.h BEFORE windows.h
 * ============================================ */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <sys/timeb.h>

/* ============================================
 * SECTION 2: POSIX to Windows Type Mappings
 * ============================================ */
typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;

typedef struct {
    int detach_state;
} pthread_attr_t;

#define PTHREAD_CREATE_DETACHED 1

/* ============================================
 * SECTION 3: Threading Functions
 * ============================================ */

/* Static mutex initializer - requires runtime init on Windows */
#define PTHREAD_MUTEX_INITIALIZER {0}
#define PTHREAD_MUTEX_NEEDS_INIT 1

/* Mutex operations as macros */
#define pthread_mutex_lock(m)    EnterCriticalSection(m)
#define pthread_mutex_unlock(m)  LeaveCriticalSection(m)
#define pthread_mutex_destroy(m) DeleteCriticalSection(m)

/* Function prototypes - implemented in compat.c */
int  win32_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine)(void*), void *arg);
void win32_pthread_exit(void *retval);
int  win32_pthread_attr_init(pthread_attr_t *attr);
int  win32_pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
void win32_mutex_init(pthread_mutex_t *mutex);

#define pthread_create     win32_pthread_create
#define pthread_exit       win32_pthread_exit
#define pthread_attr_init  win32_pthread_attr_init
#define pthread_attr_setdetachstate win32_pthread_attr_setdetachstate

/* ============================================
 * SECTION 4: Password Hashing (SHA256)
 * ============================================ */

/* Replace Unix crypt() with Windows SHA256 implementation */
char *win32_crypt_sha256(const char *password, const char *salt);
#define crypt(password, salt) win32_crypt_sha256(password, salt)

/* ============================================
 * SECTION 5: Process/Signal Functions
 * ============================================ */

/* Signal constants - stubs for Windows */
#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#ifndef SIGSEGV
#define SIGSEGV 11
#endif

/* Process functions */
#define getpid() ((int)GetCurrentProcessId())

/* fork() not available on Windows - return error */
#define fork() (-1)
#define wait(x) ((void)0)

/* SEH crash handler - implemented in compat.c */
void win32_setup_crash_handler(void);
LONG WINAPI win32_exception_filter(EXCEPTION_POINTERS *ExceptionInfo);

/* ============================================
 * SECTION 6: File/Directory Functions
 * ============================================ */
#define unlink(path)  _unlink(path)

/* Create directory if it doesn't exist (returns 0 on success or if exists) */
int ensure_directory(const char *path);

/* ============================================
 * SECTION 7: Network Functions
 * ============================================ */

/* Platform-independent socket write - use instead of write() on sockets */
int socket_write(int fd, const void *buf, size_t len);

/* gethostbyaddr_r replacement for Windows */
struct hostent *win32_gethostbyaddr_wrapper(const char *addr, int len, int type,
                                             struct hostent *result, char *buf,
                                             size_t buflen, struct hostent **res,
                                             int *h_errnop);

/* Macro to replace gethostbyaddr_r calls */
#define gethostbyaddr_r(addr, len, type, result, buf, buflen, res, err) \
    win32_gethostbyaddr_wrapper(addr, len, type, result, buf, buflen, res, err)

/* ============================================
 * SECTION 8: Time Functions
 * ============================================ */

/* gettimeofday implementation for Windows */
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int win32_gettimeofday(struct timeval *tp, struct timezone *tzp);
#define gettimeofday win32_gettimeofday

/* ============================================
 * SECTION 9: Misc Compatibility
 * ============================================ */

/* execl replacement using _spawnl */
#define execl(path, arg0, ...) _spawnl(_P_OVERLAY, path, arg0, __VA_ARGS__)

#else /* !WIN32 - Unix/Linux */

/* ============================================
 * Unix includes
 * ============================================ */
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <netdb.h>

/* crypt may need explicit include on some systems */
#ifndef NOCRYPT
#include <crypt.h>
#endif

/* Platform-independent socket write - on Unix just use write() */
#define socket_write(fd, buf, len) write(fd, buf, len)

/* Create directory if it doesn't exist (returns 0 on success or if exists) */
int ensure_directory(const char *path);

#endif /* WIN32 */

#endif /* COMPAT_H */
