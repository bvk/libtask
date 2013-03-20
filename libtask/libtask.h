#ifndef _LIBTASK_LIBTASK_H_
#define _LIBTASK_LIBTASK_H_

#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/wait_queue.h"
#include "libtask/util/semaphore.h"
#include "libtask/util/spinlock.h"

// Schedule the current task and give up the thread.
static inline error_t
libtask_yield() {
  libtask_task_t *current = libtask_get_task_current();
  if (!current) {
    return EINVAL;
  }
  return libtask_task_pool_schedule(current->owner);
}

#endif // _LIBTASK_LIBTASK_H_
