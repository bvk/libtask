#include <stdio.h>
#include <assert.h>

#include "libtask/task_pool.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

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
  CHECK(pthread_mutex_init(&task->mutex, NULL) == 0);

  task->complete = false;

  // Initialize the task with user function.
  task->result = 0;
  task->argument = argument;
  task->function = function;

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

  CHECK(pthread_mutex_destroy(&task->mutex) == 0);
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

  task->complete = false;
  task->result = task->function(task->argument);
  task->complete = true;

  libtask__task_pool_erase(originating_pool);
  // No task should ever reach here!
  CHECK(0);
  return NULL;
}

error_t
libtask__task_execute(libtask_task_t *task)
{
  if (task->complete) {
    assert(0);
    return EINVAL;
  }
  assert(libtask_list_empty(&task->waiting_link));

  // Take a reference to the task and its owner so that they cannot be
  // destroyed when the task is running.
  libtask_task_pool_t *owner = task->owner;
  libtask_task_ref(task);
  libtask_task_pool_ref(owner);

  // Lock the task's stack.
  CHECK(pthread_mutex_lock(&task->mutex) == 0);
  CHECK(libtask__set_task_current(task) == 0);

  CHECK(swapcontext(&task->uct_thread, &task->uct_self) == 0);

  CHECK(libtask__set_task_current(NULL) == 0);
  CHECK(pthread_mutex_unlock(&task->mutex) == 0);

  libtask_task_unref(task);
  libtask_task_pool_unref(owner);
  return 0;
}
