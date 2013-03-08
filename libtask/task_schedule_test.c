#include <assert.h>
#include <execinfo.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libtask/task_pool.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

#define NTASK 1000
#define NTHREAD 1
#define NSCHEDULE 100

#define TASK_STACK_SIZE (16 * 1024)

int
work(void *arg_)
{
  for (int i = 0; i < NSCHEDULE; i++) {
    libtask_task_yield();
  }
  return 0;
}

void *
tmain(void *arg_)
{
  libtask_task_pool_t *pool = (libtask_task_pool_t *)arg_;

  while (libtask_get_task_pool_size(pool) > 0) {
    libtask_task_t *task = NULL;
    if (libtask_task_pool_pop_front(pool, &task) == 0) {
      CHECK(libtask_task_execute(task) == 0);
    }
  }
  CHECK(libtask_get_task_pool_size(pool) == 0);
  return NULL;
}

int
main(int argc, char *argv[])
{
  CHECK(NTASK > NTHREAD);

  libtask_task_pool_t *pool = NULL;
  CHECK(libtask_task_pool_create(&pool) == 0);
  CHECK(pool);

  libtask_task_t task[NTASK];
  for (int i = 0; i < NTASK; i++) {
    CHECK(libtask_task_initialize(&task[i], work, NULL, TASK_STACK_SIZE) == 0);
    CHECK(libtask_task_pool_insert(pool, &task[i]) == 0);
    CHECK(libtask_get_task_pool_size(pool) == i + 1);
  }

  // Create threads and wait for them to finish all tasks.
  pthread_t threads[NTHREAD];
  for (int i = 0; i < NTHREAD; i++) {
    CHECK(pthread_create(&threads[i], NULL, tmain, pool) == 0);
  }
  for (int i = 0; i < NTHREAD; i++) {
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  // Destroy the tasks and the task pool.
  for (int i = 0; i < NTASK; i++) {
    CHECK(libtask_task_finalize(&task[i]) == 0);
  }
  CHECK(libtask_task_pool_unref(pool) == 0);
  return 0;
}
