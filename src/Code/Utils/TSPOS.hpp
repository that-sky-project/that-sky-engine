#ifndef __UTILS_TGCOS_HPP__
#define __UTILS_TGCOS_HPP__

#if defined(_WIN32) || defined(_WIN64)
#  define TSP_OS_WIN
#elif defined(__ANDROID__)
#  define TSP_OS_ANDROID
#  define TSP_OS_POSIX
#elif defined(__APPLE__) && defined(__MACH__)
#  define TSP_OS_IOS
#  define TSP_OS_POSIX
#else
// Unknown OS.
#  error "Unknown platform."
#endif

#if defined(TSP_OS_WIN)
#  include <Windows.h>
#endif

#if defined(TSP_OS_POSIX)
#  include <pthread.h>
#endif

class Lock {
public:
  ~Lock() { DeleteCriticalSection(&m_mutex); }
  Lock() { InitializeCriticalSection(&m_mutex); }

  Lock(const Lock &) = delete;
  Lock(Lock &&) = delete;
  Lock &operator=(const Lock &) = delete;

#if defined(TSP_OS_WIN)

public:
  inline void Initialize() { }
  inline void Terminate() { }
  inline void BeginLock() { EnterCriticalSection(&m_mutex); }
  inline void EndLock() { LeaveCriticalSection(&m_mutex); }

private:
  CRITICAL_SECTION m_mutex;

#elif defined(TSP_OS_POSIX)

public:
  inline void Initialize() {
    SkyAssertMsg(!m_initialized, "Lock::Initialize: already initialized!");
    m_initialized = !pthread_mutex_init(&m_mutex, 0LL);
    SkyAssertMsg(m_initialized, "Lock::Initialize: failed to create mutex");
  }

  inline void Terminate() {
    if (m_initialized && pthread_mutex_destroy(&m_mutex))
      m_initialized = false;
  }

  inline void BeginLock() {
    SkyAssertMsg(m_initialized, "Lock::BeginLock: not actually initialized!");
    int ret = pthread_mutex_lock(a1);
    SkyAssertMsg(ret == 0, "Lock::BeginLock: error %i (%s)", ret, strerror(ret));
  }

  inline void EndLock() {
    SkyAssertMsg(m_initialized, "Lock::EndLock: not actually initialized!");
    int ret = pthread_mutex_unlock(a1);
    SkyAssertMsg(ret == 0, "Lock::EndLock: error %i (%s)", ret, strerror(ret));
  }

private:
  pthread_mutex_t m_mutex;
  bool m_initialized = false;

#endif // #if defined(TSP_OS_WIN)

};

class RwLock {
public:
  ~RwLock() { }
  RwLock() { }

  RwLock(const RwLock &) = delete;
  RwLock(RwLock &&) = delete;
  RwLock &operator=(const RwLock &) = delete;

  void Initialize() { }
  void Terminate() { }
  void BeginReadLock() { }
  void EndReadLock() { }
  void BeginWriteReadLock() { }
  void EndWriteReadLock() { }

private:

};

#endif
