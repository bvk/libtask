#ifndef _LIBTASK_LIBTASK_H_
#define _LIBTASK_LIBTASK_H_

#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/util/semaphore.h"
#include "libtask/util/spinlock.h"
#include "libtask/util/condition.h"

// Schedule the current task and give up the thread.
static inline error_t
libtask_yield() {
  libtask_task_t *current = libtask_get_task_current();
  if (!current) {
    return EINVAL;
  }
  return libtask_task_pool_schedule(current->owner);
}

// Get the current thread's id.
static inline int32_t
libtask_thread_id()
{
  pid_t tid;
  tid = syscall(SYS_gettid);
  return tid;
}

// Get the current time in microseconds.
static inline int64_t
libtask_now_usecs()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t) (tv.tv_sec) * 1000000 + tv.tv_usec;
}

#endif // _LIBTASK_LIBTASK_H_
