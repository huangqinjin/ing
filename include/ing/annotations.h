#ifndef ING_ANNOTATIONS
#define ING_ANNOTATIONS

#define VERSION(major, minor, patch) (((unsigned)major << 24) + ((unsigned)minor << 8) + (unsigned)patch)
#define SINCE(major, minor, ...)
#define DEPRECATED(major, minor, ...)

#if defined(_WIN32) || defined(__CYGWIN__)
# define IMPORT __declspec(dllimport)
# define EXPORT __declspec(dllexport)
#else
# define IMPORT __attribute__((visibility("default")))
# define EXPORT __attribute__((visibility("default")))
#endif

#define THREAD_SAFE
#define THREAD_AFFINITY(...)


// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
#if defined(__clang__)

#define CAPABILITY(x)                  __attribute__((capability(x)))
#define SCOPED_CAPABILITY              __attribute__((scoped_lockable)
#define GUARDED_BY(x)                  __attribute__((guarded_by(x)))
#define PT_GUARDED_BY(x)               __attribute__((pt_guarded_by(x)))
#define ACQUIRED_BEFORE(...)           __attribute__((acquired_before(__VA_ARGS__)))
#define ACQUIRED_AFTER(...)            __attribute__((acquired_after(__VA_ARGS__)))
#define REQUIRES(...)                  __attribute__((requires_capability(__VA_ARGS__)))
#define REQUIRES_SHARED(...)           __attribute__((requires_shared_capability(__VA_ARGS__)))
#define ACQUIRE(...)                   __attribute__((acquire_capability(__VA_ARGS__)))
#define ACQUIRE_SHARED(...)            __attribute__((acquire_shared_capability(__VA_ARGS__)))
#define RELEASE(...)                   __attribute__((release_capability(__VA_ARGS__)))
#define RELEASE_SHARED(...)            __attribute__((release_shared_capability(__VA_ARGS__)))
#define RELEASE_GENERIC(...)           __attribute__((release_generic_capability(__VA_ARGS__)))
#define TRY_ACQUIRE(...)               __attribute__((try_acquire_capability(__VA_ARGS__)))
#define TRY_ACQUIRE_SHARED(...)        __attribute__((try_acquire_shared_capability(__VA_ARGS__)))
#define EXCLUDES(...)                  __attribute__((locks_excluded(__VA_ARGS__)))
#define ASSERT_CAPABILITY(x)           __attribute__((assert_capability(x)))
#define ASSERT_SHARED_CAPABILITY(x)    __attribute__((assert_shared_capability(x)))
#define RETURN_CAPABILITY(x)           __attribute__((lock_returned(x)))
#define NO_THREAD_SAFETY_ANALYSIS      __attribute__((no_thread_safety_analysis))

#else

#define CAPABILITY(x)
#define SCOPED_CAPABILITY
#define GUARDED_BY(x)
#define PT_GUARDED_BY(x)
#define ACQUIRED_BEFORE(...)
#define ACQUIRED_AFTER(...)
#define REQUIRES(...)
#define REQUIRES_SHARED(...)
#define ACQUIRE(...)
#define ACQUIRE_SHARED(...)
#define RELEASE(...)
#define RELEASE_SHARED(...)
#define RELEASE_GENERIC(...)
#define TRY_ACQUIRE(...)
#define TRY_ACQUIRE_SHARED(...)
#define EXCLUDES(...)
#define ASSERT_CAPABILITY(x)
#define ASSERT_SHARED_CAPABILITY(x)
#define RETURN_CAPABILITY(x)
#define NO_THREAD_SAFETY_ANALYSIS

#endif


#endif //ING_ANNOTATIONS
