#include <assert.h>
#include <semaphore.h>
#include <stdio.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

error_t
libtask_task_pool_initialize(libtask_task_pool_t *pool)
{
  pool->ntasks = 0;
  libtask_spinlock_initialize(&pool->spinlock);
  libtask_list_initialize(&pool->task_list);
  libtask_list_initialize(&pool->thread_list);
  libtask_list_initialize(&pool->waiting_list);
  libtask_condition_initialize(&pool->waiting_condition, &pool->spinlock);

  libtask_refcount_initialize(&pool->refcount);
  return 0;
}

error_t
libtask_task_pool_finalize(libtask_task_pool_t *pool)
{
  assert(libtask_refcount_count(&pool->refcount) <= 1);
  assert(libtask_list_empty(&pool->task_list));
  assert(libtask_list_empty(&pool->waiting_list));
  assert(libtask_list_empty(&pool->thread_list));

  libtask_condition_finalize(&pool->waiting_condition);
  libtask_spinlock_finalize(&pool->spinlock);
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

  libtask_spinlock_lock(&task_pool->spinlock);
  task_pool->ntasks++;
  libtask_task_ref(task);
  libtask_task_pool_ref(task_pool);
  libtask_list_push_back(&task_pool->task_list, &task->originating_pool_link);

  task->owner = libtask_task_pool_ref(task_pool);
  libtask_list_push_back(&task_pool->waiting_list, &task->waiting_link);
  libtask_condition_signal(&task_pool->waiting_condition);
  libtask_spinlock_unlock(&task_pool->spinlock);
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

  libtask_spinlock_lock(&task_pool->spinlock);
  task_pool->ntasks--;
  libtask_list_erase(&task->originating_pool_link);
  libtask_task_unref(task);
  libtask_task_pool_unref(task_pool);
  libtask_spinlock_unlock(&task_pool->spinlock);

  libtask_task_pool_unref(task->owner);
  task->owner = NULL;
  libtask__task_suspend(); // Should never return!
  assert(0);
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

  libtask_spinlock_lock(&task_pool->spinlock);
  libtask_list_push_back(&task_pool->waiting_list,
			 &current_task->waiting_link);
  libtask_condition_signal(&task_pool->waiting_condition);
  libtask_spinlock_unlock(&task_pool->spinlock);

  libtask__task_suspend();
  return 0;
}

static error_t
libtask__task_pool_run(libtask_task_pool_t *task_pool)
{
  assert(libtask_spinlock_status(&task_pool->spinlock) == false);

  if (libtask_list_empty(&task_pool->waiting_list)) {
    return ENOENT;
  }

  libtask_list_t *link = libtask_list_pop_front(&task_pool->waiting_list);
  libtask_task_t *task = libtask_list_entry(link, libtask_task_t,
					    waiting_link);

  libtask_spinlock_unlock(&task_pool->spinlock);
  libtask__task_execute(task);
  libtask_spinlock_lock(&task_pool->spinlock);
  return 0;
}

typedef struct {
  libtask_list_t link;
  pthread_t pthread;
} thread_entry_t;

void *
libtask__task_pool_main(void *arg_)
{
  libtask_task_pool_t *task_pool = (libtask_task_pool_t *)arg_;

  thread_entry_t entry;
  entry.pthread = pthread_self();
  libtask_list_initialize(&entry.link);

  // Enqueue the current thread into task-pool's thread list and keep
  // executing tasks from the task-pool until somebody signals to stop
  // by unlinking from the thread list.

  libtask_spinlock_lock(&task_pool->spinlock);
  libtask_list_push_back(&task_pool->thread_list, &entry.link);
  while (!libtask_list_empty(&entry.link)) {
    if (libtask_list_empty(&task_pool->waiting_list)) {
      libtask_condition_wait(&task_pool->waiting_condition);
    }
    libtask__task_pool_run(task_pool);
  }
  libtask_spinlock_unlock(&task_pool->spinlock);

  // Release the task-pool reference taken when pthread is created.
  libtask_task_pool_unref(task_pool);
  return NULL;
}

error_t
libtask_task_pool_execute(libtask_task_pool_t *task_pool)
{
  if (libtask_get_task_current()) {
    return EINVAL;
  }
  libtask_task_pool_ref(task_pool);
  libtask__task_pool_main(task_pool);
  return 0;
}

error_t
libtask_task_pool_start(libtask_task_pool_t *task_pool, pthread_t *pthreadp)
{
  pthread_t pthread;
  error_t error = pthread_create(&pthread, NULL, libtask__task_pool_main,
				 task_pool);
  if (error) {
    return error;
  }

  // Take a reference on the task-pool.
  libtask_task_pool_ref(task_pool);
  *pthreadp = pthread;
  return 0;
}

error_t
libtask_task_pool_stop(libtask_task_pool_t *task_pool, pthread_t pthread)
{
  pthread_t self = pthread_self();
  if (pthread_equal(self, pthread)) {
    return EINVAL;
  }

  libtask_spinlock_lock(&task_pool->spinlock);
  libtask_list_t *iter = libtask_list_front(&task_pool->thread_list);
  while (iter != &task_pool->thread_list) {
    thread_entry_t *entry = libtask_list_entry(iter, thread_entry_t, link);
    if (pthread_equal(entry->pthread, pthread)) {
      libtask_list_erase(&entry->link);
      // A signal may not wake up the desired thread, so wake up all
      // threads.
      libtask_condition_broadcast(&task_pool->waiting_condition);
      libtask_spinlock_unlock(&task_pool->spinlock);
      return 0;
    }
    iter = iter->next;
  }
  libtask_spinlock_unlock(&task_pool->spinlock);
  return ENOENT;
}
