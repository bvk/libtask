#ifndef _LIBTASK_WAIT_QUEUE_H_
#define _LIBTASK_WAIT_QUEUE_H_

#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include "libtask/list.h"
#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/util/spinlock.h"

typedef struct {
  libtask_spinlock_t spinlock;
  libtask_list_t waiting_list;
} libtask_wait_queue_t;

// Initialize a wait queue.
//
// wq: The wait-queue.
//
// Returns zero.
static inline error_t
libtask_wait_queue_initialize(libtask_wait_queue_t *wq) {
  libtask_spinlock_initialize(&wq->spinlock);
  libtask_list_initialize(&wq->waiting_list);
  return 0;
}

// Destroy a wait queue. Wait queue must be empty.
//
// wq: The wait-queue.
//
// Returns zero.
static inline error_t
libtask_wait_queue_finalize(libtask_wait_queue_t *wq) {
  libtask_spinlock_finalize(&wq->spinlock);
  return 0;
}

// Make current task sleep on a wait queue.
//
// wq: The wait-queue.
//
// spinlock: Optional spinlock to unlock before sleeping and lock when
//           task wakes up.
//
// Returns zero when task is woken up; returns EINVAL when called from
// outside a task context.
error_t
libtask_wait_queue_sleep(libtask_wait_queue_t *wq,
			 pthread_spinlock_t *spinlock);

// Wake up all tasks sleeping on the wait queue.
//
// wq: The wait-queue.
//
// Returns zero.
error_t
libtask_wait_queue_wake_up(libtask_wait_queue_t *wq);

// Wake up only the first task sleeping on the wait queue.
//
// wq: The wait-queue.
//
// Returns zero.
error_t
libtask_wait_queue_wake_up_first(libtask_wait_queue_t *wq);

#endif // _LIBTASK_WAIT_QUEUE_H_
