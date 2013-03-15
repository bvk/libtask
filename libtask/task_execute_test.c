#include <assert.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

#define NYIELD 1000
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

  CHECK(libtask_task_pool_execute(&task_pool) == 0);
  CHECK(counter == NYIELD);

  CHECK(libtask_task_unref(&task) == 0);
  CHECK(libtask_task_pool_unref(&task_pool) == 0);
  return 0;
}
