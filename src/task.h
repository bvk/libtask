#ifndef _LIBTASK_SRC_TASK_H_
#define _LIBTASK_SRC_TASK_H_

#include <ucontext.h>
#include <errno.h>
#include <error.h>
#include <pthread.h>
#include <stdint.h>

#include "list.h"
#include "refcount.h"

// Task
//
// A Task is a concurrently executing entity similar to threads.  It
// has a stack and control of execution, but allows the control to
// switch to other tasks and back as and when necessary.  When control
// moves from one task to another, old task retains the current state
// and control can resume forward later.  The main benefit of tasks is
// that control can be shared between multiple tasks at will without
// "returning", which allows asynchronous calls to be scheduled to
// other threads without using callbacks.  For example, with tasks, io
// operations can be scheduled to designated io threads as follows:
//
// int some_task(...) {
//   ...
//   int r = ASYNC(read(fd, buffer, size));
//   ...
// }
//
// When the compute thread's control reaches to ASYNC statement, it
// can switch to another task and return back to the this task when an
// io thread has finished the "read" operation asynchronously.

typedef struct libtask_task {
  // Number of references to the task.
  libtask_refcount_t refcount;

  // Memory for saving and restoring stack contexts. The uct_self
  // member contains the latest context of the task and the uct_thread
  // contains the context of the current thread that is executing this
  // task.  So, a swapcontext(self, thread) returns control back to
  // the thread where as a swapcontext(thread, self) resumes the task.
  ucontext_t uct_self;
  ucontext_t uct_thread;

  // Mutex to ensure no two threads operate on the same stack at once.
  pthread_mutex_t mutex;

  // Normally, runnable tasks wait in the task-pools and one or more
  // threads pick up tasks from the task-pools for execution. Below
  // two members contain the current task-pool where task is currently
  // waiting.
  libtask_list_t waiting_link;
  struct libtask_task_pool *task_pool;

  // Every task is created with its own stack. These two members refer
  // to the stack location and the size. Note that different tasks can
  // have different stack sizes.
  char *stack;
  int32_t nbytes;

  // These members refer to the task's function definition, its
  // argument and the result.
  int result;
  void *argument;
  int (*function)(void *);

  // All tasks are members of a global task list.  It is intended to
  // be used from gdb to inspect tasks and their stacks.
  libtask_list_t all_task_link;

  // Flags indicating the task state.
  bool complete;
} libtask_task_t;

// Initialize a task variable (on stack).
//
// task: Task to initialize.
//
// function: Address of the function that the task executes.
//
// argument: Argument for the function when the task is executed.
//
// stack_size: Size of the stack (in bytes) to use for executing the task.
//
// Returns 0 on success and ENOMEM on out of memory.
error_t
libtask_task_initialize(libtask_task_t *task,
			int (*function)(void *),
			void *argument,
			int32_t stack_size);

// Destroy a task!  A task cannot be destroyed while it is waiting in
// a task-pool and a task cannot destroy itself.  So, a task can be
// destroyed only when it has finished execution or is not part of any
// task-pool.
//
// task: Task to be destroy.
//
// Returns 0 on success. Returns EINVAL if task is part of any task-pool or
// task is trying to destroy itself.
error_t
libtask_task_finalize(libtask_task_t *task);

// Create a task on heap.
//
// taskp: Address of task pointer where new task has to be returned.
//
// Returns zero on success and ENOMEM on failure.
error_t
libtask_task_create(libtask_task_t **taskp,
		    int (*function)(void *),
		    void *argument,
		    int32_t stack_size);

// Take a reference.
//
// task: Task whose reference count is incremented.
//
// Returns the input task.
static inline libtask_task_t *
libtask_task_ref(libtask_task_t *task) {
  libtask_refcount_inc(&task->refcount);
  return task;
}

// Release a task reference and destroy it if necessary.
//
// task: Task to unreference and destroy.
//
// Returns the number of references left.
static inline int32_t
libtask_task_unref(libtask_task_t *task) {
  int32_t nref;
  libtask_refcount_dec(&task->refcount, libtask_task_finalize, task, &nref);
  return nref;
}

// Execute a task!  Note that a task that is not part of any task-pool
// can also be executed, but it may return early, for example, on a
// libtask_task_yield.
//
// task: The task to execute.
//
// Returns zero.
error_t
libtask_task_execute(libtask_task_t *task);

// Schedule a task back into its task-pool.  If task has no task-pool
// this acts like a no-op.
//
// task: Task to reschedule.
//
// Returns zero.
error_t
libtask_task_schedule(libtask_task_t *task);

// Pause the current task!
//
// Returns zero on success or EINVAL when called from a non-task
// context.
error_t
libtask_task_suspend(void);

// Pause and reschdule current task!  When a task is paused, control
// resumes from the location task is executed (which would be a
// libtask_task_execute function call.)  If task doesn't belong to any
// task-pool, then it is stopped until another libtask_task_execute is
// performed.
//
// Returns 0 on success. Returns EINVAL when called from a non-task
// context.
error_t
libtask_task_yield(void);

//
// Helper functions
//

// Get the current task or NULL.
libtask_task_t *libtask_get_task_current(void);

// Print all tasks' stack traces.
void libtask_print_all_tasks(void);

// Returns total number of tasks alive.
int32_t libtask_count_all_tasks(void);

#endif // _LIBTASK_SRC_TASK_H_
