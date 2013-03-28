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

#include "libtask/task_pool.h"
#include "libtask/log.h"

// Pthread key that keeps track of current task.
static pthread_key_t current_task_key;

// Pthread once initializations for this module.
static error_t pthread_once_error = 0;
static pthread_once_t pthread_once_control = PTHREAD_ONCE_INIT;

void libtask_task_once()
{
  if ((pthread_once_error = pthread_key_create(&current_task_key, NULL))) {
    return;
  }

  return;
}

static inline error_t
libtask__set_task_current(libtask_task_t *task)
{
  int result;
  result = pthread_setspecific(current_task_key, task);
  assert(result == 0);

  return 0;
}

libtask_task_t *
libtask_get_task_current(void)
{
  return (libtask_task_t *)pthread_getspecific(current_task_key);
}

static error_t
initialize(libtask_task_t *task,
	   struct libtask_task_pool *task_pool,
	   int (*function)(void *),
	   void *argument,
	   int32_t stack_size)
{
  CHECK(pthread_once(&pthread_once_control, libtask_task_once) == 0);
  if (pthread_once_error) {
    return pthread_once_error;
  }

  char *stack = (char *)malloc(stack_size);
  if (!stack) {
    return ENOMEM;
  }

  task->stack = stack;
  task->nbytes = stack_size;
  task->argument = argument;
  task->function = function;
  libtask_spinlock_initialize(&task->stack_spinlock);

  task->result = 0;
  task->complete = false;
  libtask_spinlock_initialize(&task->completed_spinlock);
  libtask_condition_initialize(&task->completed, &task->completed_spinlock);

  CHECK(getcontext(&task->uct_self) == 0);
  task->uct_self.uc_stack.ss_sp = task->stack;
  task->uct_self.uc_stack.ss_size = task->nbytes;
  task->uct_self.uc_link = NULL;

  void *libtask__task_main(libtask_task_t *task);
  makecontext(&task->uct_self, (void(*)())libtask__task_main, 1, task);

  task->owner = NULL;
  libtask_list_initialize(&task->waiting_link);
  libtask_list_initialize(&task->originating_pool_link);
  return 0;
}

error_t
libtask_task_initialize(libtask_task_t *task,
			struct libtask_task_pool *task_pool,
			int (*function)(void *),
			void *argument,
			int32_t stack_size)
{
  error_t error = initialize(task, task_pool, function, argument, stack_size);
  if (error != 0) {
    return error;
  }

  libtask_refcount_initialize(&task->refcount);
  libtask__task_pool_insert(task_pool, task);
  return 0;
}

error_t
libtask_task_finalize(libtask_task_t *task)
{
  assert(libtask_get_task_current() != task);
  assert(libtask_refcount_count(&task->refcount) <= 1);

  CHECK(task->owner == NULL);
  CHECK(libtask_list_empty(&task->waiting_link));
  CHECK(libtask_list_empty(&task->originating_pool_link));

  libtask_condition_finalize(&task->completed);
  libtask_spinlock_finalize(&task->completed_spinlock);
  libtask_spinlock_finalize(&task->stack_spinlock);

  free(task->stack);
  task->stack = NULL;
  return 0;
}

error_t
libtask_task_create(libtask_task_t **taskp,
		    libtask_task_pool_t *task_pool,
		    int (*function)(void *),
		    void *argument,
		    int32_t stack_size)
{
  libtask_task_t *task = (libtask_task_t *)malloc(sizeof(libtask_task_t));
  if (!task) {
    return ENOMEM;
  }

  error_t error = initialize(task, task_pool, function, argument, stack_size);
  if (error != 0) {
    free(task);
    return error;
  }

  libtask_refcount_create(&task->refcount);
  libtask__task_pool_insert(task_pool, task);
  *taskp = task;
  return 0;
}

error_t
libtask_task_wait(libtask_task_t *task)
{
  libtask_spinlock_lock(&task->completed_spinlock);
  while (task->complete == false) {
    libtask_condition_wait(&task->completed);
  }
  libtask_spinlock_unlock(&task->completed_spinlock);
  return 0;
}

error_t
libtask__task_suspend()
{
  libtask_task_t *task = libtask_get_task_current();
  if (!task) {
    return EINVAL;
  }
  return swapcontext(&task->uct_self, &task->uct_thread) == -1 ? errno : 0;
}

void *
libtask__task_main(libtask_task_t *task)
{
  libtask_task_pool_t *originating_pool = libtask_get_task_pool_current();

  int result = task->function(task->argument);

  libtask_spinlock_lock(&task->completed_spinlock);
  task->complete = true;
  task->result = result;
  libtask_condition_broadcast(&task->completed);
  libtask_spinlock_unlock(&task->completed_spinlock);

  libtask__task_pool_erase(originating_pool);
  // No task should ever reach here!
  CHECK(0);
  return NULL;
}

error_t
libtask__task_execute(libtask_task_t *task)
{
  assert(libtask_list_empty(&task->waiting_link));

  // Take a reference to the task and its owner so that they cannot be
  // destroyed when the task is running.
  libtask_task_pool_t *owner = task->owner;
  libtask_task_ref(task);
  libtask_task_pool_ref(owner);

  // Lock the task's stack.
  libtask_spinlock_lock(&task->stack_spinlock);
  CHECK(libtask__set_task_current(task) == 0);

  CHECK(swapcontext(&task->uct_thread, &task->uct_self) == 0);

  CHECK(libtask__set_task_current(NULL) == 0);
  libtask_spinlock_unlock(&task->stack_spinlock);

  libtask_task_unref(task);
  libtask_task_pool_unref(owner);
  return 0;
}
