#include <assert.h>

#include "libtask/task.h"
#include "libtask/task_pool.h"
#include "libtask/util/condition.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

void
libtask_condition_initialize(libtask_condition_t *cond,
			     libtask_spinlock_t *spinlock)
{
  pthread_cond_init(&cond->cond, NULL);
  pthread_mutex_init(&cond->mutex, NULL);
  cond->spinlock = spinlock;
  libtask_list_initialize(&cond->list);
}

void
libtask_condition_finalize(libtask_condition_t *cond)
{
  assert(libtask_list_empty(&cond->list));
  pthread_cond_destroy(&cond->cond);
  pthread_mutex_destroy(&cond->mutex);
}

void
libtask_condition_wait(libtask_condition_t *cond)
{
  // Spinlock must be locked before wait is called!
  assert(libtask_spinlock_status(cond->spinlock) == false);

  libtask_task_t *task = libtask_get_task_current();
  if (task) {
    // Task context!
    libtask_list_push_back(&cond->list, &task->waiting_link);
    libtask_spinlock_unlock(cond->spinlock);
    libtask__task_suspend();

  } else {
    // Pthread context!
    CHECK(pthread_mutex_lock(&cond->mutex) == 0);

    libtask_spinlock_unlock(cond->spinlock);
    pthread_cond_wait(&cond->cond, &cond->mutex);

    CHECK(pthread_mutex_unlock(&cond->mutex) == 0);
  }

  libtask_spinlock_lock(cond->spinlock);
}

static inline bool
libtask_condition_wakeup_first(libtask_condition_t *cond, libtask_list_t *list)
{
  libtask_list_t *link = libtask_list_pop_front(list);
  if (!link) {
    return false;
  }

  libtask_task_t *task = libtask_list_entry(link, libtask_task_t,
					    waiting_link);
  libtask_task_pool_t *task_pool = task->owner;
  if (&task_pool->spinlock != cond->spinlock) {
    libtask_spinlock_lock(&task_pool->spinlock);
  }

  task_pool->ntasks++;
  libtask_list_push_back(&task_pool->waiting_list, &task->waiting_link);

  if (&task_pool->spinlock != cond->spinlock) {
    libtask_spinlock_unlock(&task_pool->spinlock);
  }
  return true;
}

void
libtask_condition_signal(libtask_condition_t *cond)
{
  assert(libtask_spinlock_status(cond->spinlock) == false);

  if (libtask_condition_wakeup_first(cond, &cond->list) == false) {
    CHECK(pthread_mutex_lock(&cond->mutex) == 0);
    pthread_cond_signal(&cond->cond);
    CHECK(pthread_mutex_unlock(&cond->mutex) == 0);
  }
}

void
libtask_condition_broadcast(libtask_condition_t *cond)
{
  assert(libtask_spinlock_status(cond->spinlock) == false);

  libtask_list_t list;
  libtask_list_initialize(&list);
  libtask_list_move(&list, &cond->list);

  while (!libtask_list_empty(&list)) {
    libtask_condition_wakeup_first(cond, &list);
  }
  pthread_cond_broadcast(&cond->cond);
}
