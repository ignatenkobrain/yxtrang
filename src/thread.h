#ifndef THREAD_H
#define THREAD_H

typedef struct _lock* lock;
typedef struct _thread_pool* thread_pool;

extern lock lock_create(void);
extern void lock_lock(lock l);
extern void lock_unlock(lock l);
extern void lock_destroy(lock l);

extern int atomic_inc(int* v);	// returns pre-value
extern int atomic_dec(int* v);	// return post-value

// Run a supplied function as a one-off detached thread. The
// thread will be destroyed after this single use.

extern int thread_run(int (*f)(void*), void* data);

// Run a supplied function from a pool of threads. The thread
// will be returned to the pool for fast re-use when needed.

extern thread_pool tpool_create(int threads);
extern int tpool_start(thread_pool tp, int (*f)(void*), void* data);
extern void tpool_destroy(thread_pool tp);

#endif
