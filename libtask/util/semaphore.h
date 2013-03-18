#ifndef _LIBTASK_SEMAPHORE_H_
#define _LIBTASK_SEMAPHORE_H_

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include "libtask/list.h"
#include "libtask/task.h"
#include "libtask/task_pool.h"

typedef struct {
  pthread_spinlock_t spinlock;
  int32_t count;
  libtask_list_t waiting_list;
} libtask_semaphore_t;

// Initialize a semaphore.
//
// sem: The semaphore.
//
// Returns 0 on success and an error number on failure.
error_t
libtask_semaphore_initialize(libtask_semaphore_t *sem, int32_t count);

// Destroy a semaphore. No tasks must be waiting on the semaphore.
//
// sem: The semaphore.
//
// Returns 0 on success and an error number on failure.
error_t
libtask_semaphore_finalize(libtask_semaphore_t *sem);

// Up a semaphore.
//
// sem: The semaphore.
//
// Returns 0 on success and an error number on failure.
error_t
libtask_semaphore_up(libtask_semaphore_t *sem);

// Down a semaphore and wait if necessary.
//
// sem: The semaphore.
//
// Returns zero when task is woken up; returns EINVAL when called from
// outside the task context.
error_t
libtask_semaphore_down(libtask_semaphore_t *sem);

#endif // _LIBTASK_SEMAPHORE_H_
