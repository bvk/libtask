#ifndef _LIBTASK_SEMAPHORE_H_
#define _LIBTASK_SEMAPHORE_H_

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include "libtask/list.h"
#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/util/spinlock.h"

typedef struct {
  libtask_spinlock_t spinlock;
  int64_t count;
  libtask_list_t waiting_list;
} libtask_semaphore_t;

// Initialize a semaphore.
//
// sem: The semaphore.
//
// count: The initial value.
void
libtask_semaphore_initialize(libtask_semaphore_t *sem, int32_t count);

// Destroy a semaphore. No tasks must be waiting on the semaphore.
//
// sem: The semaphore.
void
libtask_semaphore_finalize(libtask_semaphore_t *sem);

// Up a semaphore.
//
// sem: The semaphore.
void
libtask_semaphore_up(libtask_semaphore_t *sem);

// Down a semaphore and wait if necessary. This function should be
// called only from task context.
//
// sem: The semaphore.
void
libtask_semaphore_down(libtask_semaphore_t *sem);

#endif // _LIBTASK_SEMAPHORE_H_
