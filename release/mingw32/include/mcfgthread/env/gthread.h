// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_GTHREAD_H_
#define __MCFCRT_ENV_GTHREAD_H_

// Compatibility layer for libgcc and other GCC libraries.

#include "_crtdef.h"
#include "thread_env.h"
#include "thread.h"
#include "once_flag.h"
#include "mutex.h"
#include "condition_variable.h"
#include "clocks.h"

#ifdef __GTHREADS
#	error __GTHREADS is already defined. (Thread model confliction detected?)
#endif

#define __GTHREADS 1

_MCFCRT_EXTERN_C_BEGIN

_MCFCRT_CONSTEXPR int __gthread_active_p(void) _MCFCRT_NOEXCEPT {
	return 1;
}

//-----------------------------------------------------------------------------
// Thread local storage
//-----------------------------------------------------------------------------
typedef _MCFCRT_TlsKeyHandle __gthread_key_t;

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_key_create, (__gthread_key_t *__key_ret, void (*__destructor)(void *)) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_key_delete, (__gthread_key_t __key) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(void *, __gthread_getspecific, (__gthread_key_t __key) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_setspecific, (__gthread_key_t __key, const void *__value) _MCFCRT_NOEXCEPT);

//-----------------------------------------------------------------------------
// Once
//-----------------------------------------------------------------------------
typedef _MCFCRT_OnceFlag __gthread_once_t;

#define __GTHREAD_ONCE_INIT    { 0 }

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_once, (__gthread_once_t *__flag, void (*__func)(void)) _MCFCRT_NOEXCEPT);

//-----------------------------------------------------------------------------
// Mutex
//-----------------------------------------------------------------------------
typedef _MCFCRT_Mutex __gthread_mutex_t;

#define __GTHREAD_MUTEX_INIT            { 0 }
#define __GTHREAD_MUTEX_INIT_FUNCTION   __gthread_mutex_init_function

extern __MCFCRT_RENAMED_PREFIXED(void, __gthread_mutex_init_function, (__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_mutex_destroy, (__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT);

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_mutex_trylock, (__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_mutex_lock, (__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_mutex_unlock, (__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT);

//-----------------------------------------------------------------------------
// Recursive mutex
//-----------------------------------------------------------------------------
typedef struct __MCFCRT_tagGthreadRecursiveMutex {
	volatile _MCFCRT_STD uintptr_t __owner;
	_MCFCRT_STD size_t __count;
	__gthread_mutex_t __mutex;
} __gthread_recursive_mutex_t;

#define __GTHREAD_RECURSIVE_MUTEX_INIT            { 0, 0, { 0 } }
#define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION   __gthread_recursive_mutex_init_function

extern __MCFCRT_RENAMED_PREFIXED(void, __gthread_recursive_mutex_init_function, (__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_recursive_mutex_destroy, (__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT);

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_recursive_mutex_trylock, (__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_recursive_mutex_lock, (__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_recursive_mutex_unlock, (__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT);

//-----------------------------------------------------------------------------
// Condition variable
//-----------------------------------------------------------------------------
#define __GTHREAD_HAS_COND 1

typedef _MCFCRT_ConditionVariable __gthread_cond_t;

#define __GTHREAD_COND_INIT             { 0 }
#define __GTHREAD_COND_INIT_FUNCTION    __gthread_cond_init_function

extern __MCFCRT_RENAMED_PREFIXED(void, __gthread_cond_init_function, (__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_cond_destroy, (__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT);

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_cond_wait, (__gthread_cond_t *__cond, __gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_cond_wait_recursive, (__gthread_cond_t *__cond, __gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_cond_signal, (__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_cond_broadcast, (__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT);

//-----------------------------------------------------------------------------
// Thread
//-----------------------------------------------------------------------------
#define __GTHREADS_CXX0X 1

typedef _MCFCRT_STD uintptr_t __gthread_t;

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_create, (__gthread_t *__tid_ret, void *(*__proc)(void *), void *__param) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_join, (__gthread_t __tid, void **restrict __exit_code_ret) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_detach, (__gthread_t __tid) _MCFCRT_NOEXCEPT);

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_equal, (__gthread_t __tid1, __gthread_t __tid2) _MCFCRT_NOEXCEPT);
__attribute__((__const__))
extern __MCFCRT_RENAMED_PREFIXED(__gthread_t, __gthread_self, (void) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_yield, (void) _MCFCRT_NOEXCEPT);

typedef struct __MCFCRT_tagGthreadTime {
	_MCFCRT_STD int64_t __seconds;
	long __nanoseconds;
} __gthread_time_t;

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_mutex_timedlock, (__gthread_mutex_t *restrict __mutex, const __gthread_time_t *restrict __timeout) _MCFCRT_NOEXCEPT);
extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_recursive_mutex_timedlock, (__gthread_recursive_mutex_t *restrict __recur_mutex, const __gthread_time_t *restrict __timeout) _MCFCRT_NOEXCEPT);

extern __MCFCRT_RENAMED_PREFIXED(int, __gthread_cond_timedwait, (__gthread_cond_t *restrict __cond, __gthread_mutex_t *restrict __mutex, const __gthread_time_t *restrict __timeout) _MCFCRT_NOEXCEPT);

_MCFCRT_EXTERN_C_END

#ifndef __MCFCRT_GTHREAD_INLINE_OR_EXTERN
#	define __MCFCRT_GTHREAD_INLINE_OR_EXTERN     __attribute__((__gnu_inline__)) extern inline
#endif
#include "_gthread_inl.h"

#endif
