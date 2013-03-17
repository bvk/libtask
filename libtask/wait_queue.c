#include "libtask/wait_queue.h"

error_t
libtask_wait_queue_sleep(libtask_wait_queue_t *wq,
			 pthread_spinlock_t *spinlock) {
  libtask_task_t *task = libtask_get_task_current();
  if (!task) {
    return EINVAL;
  }

  pthread_spin_lock(&wq->spinlock);
  libtask_list_push_back(&wq->waiting_list, &task->waiting_link);
  pthread_spin_unlock(&wq->spinlock);

  if (spinlock) {
    pthread_spin_unlock(spinlock);
  }

  libtask__task_suspend();

  if (spinlock) {
    pthread_spin_lock(spinlock);
  }
  return 0;
}

error_t
libtask_wait_queue_wake_up(libtask_wait_queue_t *wq) {
  libtask_list_t list;
  libtask_list_initialize(&list);

  pthread_spin_lock(&wq->spinlock);
  libtask_list_move(&list, &wq->waiting_list);
  pthread_spin_unlock(&wq->spinlock);

  while (!libtask_list_empty(&list)) {
    libtask_list_t *link = libtask_list_pop_front(&list);
    libtask_task_t *task = libtask_list_entry(link, libtask_task_t,
					      waiting_link);
    assert(task);

    libtask_task_pool_t *task_pool = task->owner;
    pthread_spin_lock(&task_pool->spinlock);
    task_pool->ntasks++;
    libtask_list_push_back(&task_pool->waiting_list, &task->waiting_link);
    pthread_spin_unlock(&task_pool->spinlock);
  }
  return 0;
}

error_t
libtask_wait_queue_wake_up_first(libtask_wait_queue_t *wq) {
  pthread_spin_lock(&wq->spinlock);
  libtask_list_t *link = libtask_list_pop_front(&wq->waiting_list);
  pthread_spin_unlock(&wq->spinlock);
  if (!link) {
    return 0;
  }
  libtask_task_t *task = libtask_list_entry(link, libtask_task_t,
					    waiting_link);
  libtask_task_pool_t *task_pool = task->owner;
  pthread_spin_lock(&task_pool->spinlock);
  task_pool->ntasks++;
  libtask_list_push_back(&task_pool->waiting_list, &task->waiting_link);
  pthread_spin_unlock(&task_pool->spinlock);
  return 0;
}
