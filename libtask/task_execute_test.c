#include <assert.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)
#define DEBUG(fmt,...) printf (fmt, ##__VA_ARGS__)

#define NTHREADS 10
#define NYIELD 10000
#define TASK_STACK_SIZE (16 * 1024)

int
work(void *arg_)
{
  volatile int *count = (volatile int *)arg_;
  for (int i = 0; i < NYIELD; i++) {
    *count += 1;

    CHECK(libtask_yield() == 0);
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  libtask_task_pool_t task_pool;
  CHECK(libtask_task_pool_initialize(&task_pool) == 0);

  int counter = 0;
  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, &task_pool, work, &counter,
				TASK_STACK_SIZE) == 0);

  pthread_t pthread[NTHREADS];
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_start(&task_pool, &pthread[i]) == 0);
  }
  CHECK(libtask_task_wait(&task) == 0);
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_stop(&task_pool, pthread[i]) == 0);
    CHECK(pthread_join(pthread[i], NULL) == 0);
  }

  CHECK(counter == NYIELD);

  CHECK(libtask_task_unref(&task) == 0);
  CHECK(libtask_task_pool_unref(&task_pool) == 0);
  return 0;
}
