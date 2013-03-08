#include <assert.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "task.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

#define NYIELD 1000
#define TASK_STACK_SIZE (16 * 1024)

int
work(void *arg_)
{
  volatile int *count = (volatile int *)arg_;
  for (int i = 0; i < NYIELD; i++) {
    *count += 1;

    CHECK(libtask_task_yield() == 0);
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  int counter = 0;
  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, work, &counter, TASK_STACK_SIZE) == 0);

  for (int i = 0; i < NYIELD; i++) {
    libtask_task_execute(&task);
  }
  CHECK(counter == NYIELD);
  CHECK(libtask_task_finalize(&task) == 0);
  return 0;
}
