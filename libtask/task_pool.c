#include <assert.h>
#include <stdio.h>

#include "libtask/task_pool.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

error_t
libtask_task_pool_initialize(libtask_task_pool_t *pool)
{
  int result = sem_init(&pool->semaphore, 0 /* pshared */, 0 /* value */);
  if (result != 0) {
    return errno;
  }

  pool->ntasks = 0;
  libtask_list_initialize(&pool->task_list);
  libtask_list_initialize(&pool->waiting_list);
  CHECK(pthread_spin_init(&pool->spinlock, PTHREAD_PROCESS_PRIVATE) == 0);

  libtask_refcount_initialize(&pool->refcount);
  return 0;
}

error_t
libtask_task_pool_finalize(libtask_task_pool_t *pool)
{
  assert(libtask_refcount_count(&pool->refcount) <= 1);
  assert(libtask_list_empty(&pool->task_list));
  assert(libtask_list_empty(&pool->waiting_list));

  CHECK(sem_destroy(&pool->semaphore) == 0);
  CHECK(pthread_spin_destroy(&pool->spinlock) == 0);
  return 0;
}

error_t
libtask_task_pool_create(libtask_task_pool_t **new_poolp)
{
  libtask_task_pool_t *pool =
    (libtask_task_pool_t *) calloc(sizeof(libtask_task_pool_t), 1);
  if (!pool) {
    return ENOMEM;
  }

  error_t error = libtask_task_pool_initialize(pool);
  if (error) {
    free(pool);
    return error;
  }

  libtask_refcount_create(&pool->refcount);
  *new_poolp = pool;
  return 0;
}

void
libtask__task_pool_insert(libtask_task_pool_t *task_pool,
			  libtask_task_t *task)
{
  assert(libtask_list_empty(&task->originating_pool_link));

  pthread_spin_lock(&task_pool->spinlock);
  task_pool->ntasks++;
  libtask_task_ref(task);
  libtask_task_pool_ref(task_pool);
  libtask_list_push_back(&task_pool->task_list, &task->originating_pool_link);

  task_pool->ntasks++;
  task->owner = libtask_task_pool_ref(task_pool);
  libtask_list_push_back(&task_pool->waiting_list, &task->waiting_link);
  pthread_spin_unlock(&task_pool->spinlock);
}

void
libtask__task_pool_erase(libtask_task_pool_t *task_pool)
{
  libtask_task_t *task = libtask_get_task_current();
  assert(task);
  assert(task->complete);

  if (task->owner != task_pool) {
    libtask_task_pool_schedule(task_pool);
  }

  pthread_spin_lock(&task_pool->spinlock);
  task_pool->ntasks--;
  libtask_list_erase(&task->originating_pool_link);
  libtask_task_unref(task);
  libtask_task_pool_unref(task_pool);
  pthread_spin_unlock(&task_pool->spinlock);

  libtask_task_pool_unref(task->owner);
  task->owner = NULL;
  libtask__task_suspend(); // Should never return!
  assert(0);
}

error_t
libtask_task_pool_execute(libtask_task_pool_t *task_pool)
{
  pthread_spin_lock(&task_pool->spinlock);
  while (task_pool->ntasks > 0) {
    libtask_list_t *link = libtask_list_pop_front(&task_pool->waiting_list);
    libtask_task_t *task = NULL;
    if (link) {
      task_pool->ntasks--;
      task = libtask_list_entry(link, libtask_task_t, waiting_link);
    }
    pthread_spin_unlock(&task_pool->spinlock);
    if (task) {
      libtask__task_execute(task);
    } else {
      struct timespec ts = { 1, 0 };
      sem_timedwait(&task_pool->semaphore, &ts);
    }
    pthread_spin_lock(&task_pool->spinlock);
  }
  pthread_spin_unlock(&task_pool->spinlock);
  return 0;
}

error_t
libtask_task_pool_schedule(libtask_task_pool_t *task_pool)
{
  libtask_task_t *current_task = libtask_get_task_current();
  if (!current_task) {
    return EINVAL;
  }

  if (current_task->owner != task_pool) {
    libtask_task_pool_unref(current_task->owner);
    current_task->owner = libtask_task_pool_ref(task_pool);
  }

  pthread_spin_lock(&task_pool->spinlock);
  task_pool->ntasks++;
  libtask_list_push_back(&task_pool->waiting_list,
			 &current_task->waiting_link);
  CHECK(sem_post(&task_pool->semaphore) == 0);
  pthread_spin_unlock(&task_pool->spinlock);

  libtask__task_suspend();
  return 0;
}
