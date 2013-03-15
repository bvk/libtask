#ifndef _LIBTASK_TASK_POOL_H_
#define _LIBTASK_TASK_POOL_H_

#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include "libtask/list.h"
#include "libtask/refcount.h"
#include "libtask/task.h"

// Task Pool
//
// A task-pool is where a threads look for pending work to be
// executed.  Users may choose to create multiple task-pools and
// designate different number of threads for each.  For example, based
// on the system configuration, executing X io-operations and Y
// compute-operations in parallel may give optimal utilization, so
// users can choose to create two task-pools and assign X and Y number
// of threads to each respectively.

typedef struct libtask_task_pool {
  // Since task-pools are accessed by multiple threads, it is
  // difficult to destroy a task-pool safely.  When a thread is
  // destroying a task-pool, another thread may choose to access other
  // functions of the task-pool.  One way to destroy safely is to
  // reference count the task-pools and call destroy only when no
  // references are left.
  libtask_refcount_t refcount;

  // Tasks are kept in a unbounded list and are executed in FIFO
  // order.  Since the storage for the list is part of libtask_task_t,
  // there is no limit on number of tasks in a task-pool.  Since time
  // taken for addition and removal of a task is very small, a
  // spinlock is more appropriate.
  pthread_spinlock_t spinlock;
  int32_t ntasks;
  libtask_list_t waiting_list;
} libtask_task_pool_t;

// Initialize a task-pool created on stack.
//
// task_pool: Task-pool to initialize.
//
// Returns zero on success and ENOMEM on a failure.
error_t
libtask_task_pool_initialize(libtask_task_pool_t *task_pool);

// Destroy a task-pool. Task pool reference count must be zero.
//
// task_pool: Task pool to destroy.
//
// Returns zero.
error_t
libtask_task_pool_finalize(libtask_task_pool_t *task_pool);

// Create a task-pool on heap.
//
// task_poolp: Task-pool to initialize.
//
// Returns zero on success and ENOMEM on a failure.
error_t
libtask_task_pool_create(libtask_task_pool_t **task_poolp);

// Take a reference.
//
// task_pool: Task-pool whose reference count has to be incremented.
//
// Returns the input task-pool.
static inline libtask_task_pool_t *
libtask_task_pool_ref(libtask_task_pool_t *task_pool) {
  libtask_refcount_inc(&task_pool->refcount);
  return task_pool;
}

// Release a task-pool reference and destroy it if necessary.  If a
// task-pool contains any tasks, they will be orphaned on a destroy.
//
// task_pool: Task-pool to unreference.
//
// Returns the number of references left.
static inline int32_t
libtask_task_pool_unref(libtask_task_pool_t *task_pool) {
  int32_t nref;
  libtask_refcount_dec(&task_pool->refcount,
		       libtask_task_pool_finalize, task_pool,
		       &nref);
  return nref;
}

// Get the number of tasks in the task pool.
//
// task_pool: Task-pool with tasks.
//
// Returns number of tasks in the task-pool.
int32_t
libtask_get_task_pool_size(libtask_task_pool_t *task_pool);

// Put a task back into the task-pool.
//
// task_pool: Task-pool to put the task into.
//
// task: Task to add.
//
// Returns zero on success or EINVAL if task is removed from the task-pool.
error_t
libtask_task_pool_push_back(libtask_task_pool_t *task_pool,
			    libtask_task_t *task);

// Take a task from the task-pool for execution.
//
// task_pool: Task-pool with tasks.
//
// taskp: Task selected for execution.
//
// Returns zero on success or ENOENT when there are no pending tasks.
error_t
libtask_task_pool_pop_front(libtask_task_pool_t *task_pool,
			    libtask_task_t **taskp);

// Add a standalone task into the task-pool.
//
// task_pool: Task-pool to add the task into.
//
// task: A new task.
//
// Returns zero.
error_t
libtask_task_pool_insert(libtask_task_pool_t *task_pool,
			 libtask_task_t *task);

// Remove a task from the task-pool.
//
// task_pool: Task-pool from where task must be removed.
//
// task: The task to remove.
//
// Returns zero.
error_t
libtask_task_pool_erase(libtask_task_pool_t *task_pool,
			libtask_task_t *task);

// Switch current task to another task-pool.
//
// task_pool: Destination task-pool for the switch.
//
// old_poolp: When non-null, is stored with a reference to the old
//            task pool of the current task.
//
// Returns zero on success and EINVAL if no task is currently
// executing.
error_t
libtask_task_pool_switch(libtask_task_pool_t *task_pool,
			 libtask_task_pool_t **old_poolp);

// Execute tasks in a task-pool.
//
// This method blocks the calling thread until all tasks of the
// task-pool are completed.
//
// task-pool: The task-pool.
//
// Returns zero.
error_t
libtask_task_pool_execute(libtask_task_pool_t *task_pool);

#endif // _LIBTASK_TASK_POOL_H_
