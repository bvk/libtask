#include "libtask/util/semaphore.h"

void
libtask_semaphore_initialize(libtask_semaphore_t *sem, int32_t count)
{
  libtask_spinlock_initialize(&sem->spinlock);
  sem->count = count;
  libtask_list_initialize(&sem->waiting_list);
}

void
libtask_semaphore_finalize(libtask_semaphore_t *sem)
{
  assert(libtask_list_empty(&sem->waiting_list));
  libtask_spinlock_finalize(&sem->spinlock);
}

void
libtask_semaphore_up(libtask_semaphore_t *sem)
{
  libtask_task_t *task = NULL;

  libtask_spinlock_lock(&sem->spinlock);
  if (libtask_list_empty(&sem->waiting_list)) {
    sem->count++;
  } else {
    libtask_list_t *link = libtask_list_pop_front(&sem->waiting_list);
    task = libtask_list_entry(link, libtask_task_t, waiting_link);
  }
  libtask_spinlock_unlock(&sem->spinlock);
  if (!task) {
    return;
  }
  libtask_task_pool_t *task_pool = task->owner;
  libtask_spinlock_lock(&task_pool->spinlock);
  libtask_list_push_back(&task_pool->waiting_list, &task->waiting_link);
  libtask_condition_signal(&task_pool->waiting_condition);
  libtask_spinlock_unlock(&task_pool->spinlock);
}

void
libtask_semaphore_down(libtask_semaphore_t *sem)
{
  libtask_task_t *task = libtask_get_task_current();
  assert(task);

  libtask_spinlock_lock(&sem->spinlock);
  if (sem->count > 0) {
    sem->count--;
    libtask_spinlock_unlock(&sem->spinlock);
  } else {
    libtask_list_push_back(&sem->waiting_list, &task->waiting_link);
    libtask_spinlock_unlock(&sem->spinlock);
    libtask__task_suspend();
  }
}
