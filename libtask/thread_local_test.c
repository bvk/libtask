//
// Testcase to check that pthread_getspecific/pthread_setspecific
// functions can be used for thread local storage.
//
#include <assert.h>
#include <execinfo.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libtask/task_pool.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

#define NTHREAD 10
#define NITERATIONS 100
#define TASK_STACK_SIZE (16 * 1024)

pthread_key_t key;

static inline pid_t
thread_id()
{
  pid_t tid;
  tid = syscall(SYS_gettid);
  return tid;
}

void
switch_thread()
{
  libtask_task_yield();
}

int
work(void *arg_)
{
  int tid = thread_id();
  CHECK(pthread_key_create(&key, NULL) == 0);
  CHECK(pthread_setspecific(key, (void *)tid) == 0);

  for (int i = 0; i < NITERATIONS; i++) {
    switch_thread();

    if (tid == thread_id()) {
      CHECK(pthread_getspecific(key) == (void *)tid);
    } else {
      CHECK(pthread_getspecific(key) != (void *)tid);
    }
  }
  return 0;
}

void *
tmain(void *arg_)
{
  libtask_task_pool_t *pool = (libtask_task_pool_t *)arg_;
  CHECK(libtask_task_pool_execute(pool) == 0);
  return NULL;
}

int
main(int argc, char *argv[])
{
  libtask_task_pool_t *pool = NULL;
  CHECK(libtask_task_pool_create(&pool) == 0);
  CHECK(pool);

  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, work, NULL, TASK_STACK_SIZE) == 0);
  CHECK(libtask_task_pool_insert(pool, &task) == 0);

  // Create threads and wait for them to finish.
  pthread_t threads[NTHREAD];
  for (int i = 0; i < NTHREAD; i++) {
    CHECK(pthread_create(&threads[i], NULL, tmain, pool) == 0);
  }
  for (int i = 0; i < NTHREAD; i++) {
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  // Destroy the tasks and the task pool.
  CHECK(libtask_task_unref(&task) == 0);
  CHECK(libtask_task_pool_unref(pool) == 0);
  return 0;
}
