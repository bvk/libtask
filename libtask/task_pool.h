#ifndef _LIBTASK_TASK_POOL_H_
#define _LIBTASK_TASK_POOL_H_

#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include "libtask/list.h"
#include "libtask/refcount.h"
#include "libtask/task.h"
#include "libtask/util/spinlock.h"
#include "libtask/util/condition.h"

// Task Pool
//
// A task-pool is where a threads look for the pending work.  Users
// may choose to create multiple task-pools and designate different
// number of threads for each.  For example, based on the system
// configuration, executing X io-operations and Y compute-operations
// in parallel may give optimal utilization, so users can choose to
// create two task-pools and assign X and Y number of threads to each
// respectively.

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
  libtask_spinlock_t spinlock;

  // Lists of tasks that belong to this task-pool for inspection,
  // analysis and debugging purposes. Tasks are linked into this list
  // through their originating_pool_link.
  libtask_list_t task_list;

  // Number of tasks in this task-pool. This is the list of tasks
  // originated from this task-pool. Since tasks can switch betweek
  // task-pools, there can be more tasks than the originating in a
  // task-pool.
  int32_t ntasks;

  // List of tasks waiting for execution and the condition variable
  // that wakes up waiting threads.
  libtask_list_t waiting_list;
  libtask_condition_t waiting_condition;

  // List of threads looking for work on this task-pool.
  libtask_list_t thread_list;
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

// Execute tasks from the task-pool. Thread is blocked in executing
// and waiting for tasks in the task-pool until it is requested to
// stop.
//
// task-pool: The task-pool.
//
// Returns zero on success or EINVAL if thread is already
// executing tasks from a task-pool.
error_t
libtask_task_pool_execute(libtask_task_pool_t *task_pool);

// Create a new thread and assign it to a task-pool for execution.
//
// task-pool: The task-pool.
//
// threadp: Output variable where id of the new thread is returned.
//
// Returns zero on success or an error number on failure.
error_t
libtask_task_pool_start(libtask_task_pool_t *task_pool, pthread_t *threadp);

// Stop a thread executing tasks from a task-pool.
//
// task-pool: The task-pool.
//
// thread: Id of the thread.
//
// Returns zero on success and ENOENT if no such thread is executing
// tasks from the task-pool.
error_t
libtask_task_pool_stop(libtask_task_pool_t *task_pool, pthread_t thread);

// Schedule current task to a task-pool.
//
// task_pool: The task-pool. When this task-pool is different from
//            current task-pool, current task is switched to the given
//            task-pool.
//
// Returns 0 on success or EINVAL on a failure.
error_t
libtask_task_pool_schedule(libtask_task_pool_t *task_pool);

// Get the number of tasks in the task-pool.
//
// task_pool: The task-pool.
//
// Returns the number of tasks or zero.
static inline int32_t
libtask_get_task_pool_size(libtask_task_pool_t *task_pool)
{
  libtask_spinlock_lock(&task_pool->spinlock);
  int32_t size = task_pool->ntasks;
  libtask_spinlock_unlock(&task_pool->spinlock);
  return size;
}

// Get the current task-pool.  Returns NULL if current context doesn't
// belong to a task-pool.  Note that if task-pool address has to be
// stored then, a reference shoule be taken.
static inline libtask_task_pool_t *
libtask_get_task_pool_current(void)
{
  libtask_task_t *task = libtask_get_task_current();
  return task ? task->owner : NULL;
}

//
// Private interfaces
//

// Associate a task with the task-pool for the first time.
void
libtask__task_pool_insert(libtask_task_pool_t *task_pool,
			  libtask_task_t *task);

// Remove current task from the task-pool because it is complete.
void
libtask__task_pool_erase(libtask_task_pool_t *task_pool);

#endif // _LIBTASK_TASK_POOL_H_
