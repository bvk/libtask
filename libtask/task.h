//
// Libtask: A thread-safe coroutine library.
//
// Copyright (C) 2013  BVK Chaitanya
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef _LIBTASK_TASK_H_
#define _LIBTASK_TASK_H_

#include <pthread.h>
#include <ucontext.h>

#include "libtask/util/condition.h"
#include "libtask/util/list.h"
#include "libtask/util/refcount.h"

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

  // Every task is created with its own stack. These two members refer
  // to the stack location and the size. Note that different tasks can
  // have different stack sizes.
  //
  // When a task is moved from one task-pool to another, it is
  // possible that a thread in the destination task-pool may
  // immediately pick the task for execution. So, we need a lock to
  // protect concurrent access to the stack.
  char *stack;
  int32_t nbytes;
  libtask_spinlock_t stack_spinlock;

  // These members refer to the task's function definition, its
  // argument.
  void *argument;
  int (*function)(void *);

  // Task status, flags and the condition variable to wait for the
  // task to finish.
  int result;
  bool complete;
  libtask_condition_t completed;
  libtask_spinlock_t completed_spinlock;

  // Normally, runnable tasks wait in the task-pools and one or more
  // threads pick up tasks from the task-pools for execution. Below
  // two members contain the current task-pool where task is currently
  // waiting.
  libtask_list_t waiting_link;

  // A task is always owned by a task-pool, so that when task yields
  // the thread, it can be put back in a task-pool for later
  // execution.
  struct libtask_task_pool *owner;

  // Even though a task migrates between different task-pools during
  // its life time, it is still associated with an originating
  // task-pool for its entire life time. This task-pool is where tasks
  // can be inspected and analyzed for reporting or debugging.
  libtask_list_t originating_pool_link;

} libtask_task_t;

// Initialize a task variable (on stack).
//
// task: Task to initialize.
//
// task_pool: Task-pool this task belongs to.
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
			struct libtask_task_pool *task_pool,
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
		    struct libtask_task_pool *task_pool,
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

// Wait for a task to finish.
//
// task: Task to wait for.
//
// Returns zero.
error_t
libtask_task_wait(libtask_task_t *task);

// Get the current task. Returns NULL when called from outside the
// task context. Note that if task address has to be stored then, a
// reference should be taken.
libtask_task_t *
libtask_get_task_current(void);

//
// Private interfaces
//

// Resume a task.
error_t
libtask__task_execute(libtask_task_t *task);

// Suspend current task.
error_t
libtask__task_suspend(void);

#endif // _LIBTASK_TASK_H_
