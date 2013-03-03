#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "task_pool.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

#define NITERATIONS 1000

#define TASK_STACK_SIZE (16 * 1024)

static libtask_task_pool_t *io_pool;
static libtask_task_pool_t *cpu_pool;

static int32_t nio = 0;
static int32_t ncpu = 0;

static int32_t ntasks = 0;

int
work(void *arg_)
{
  libtask_atomic_add(&ntasks, 1);
  for (int i = 0; i < NITERATIONS; i++) {
    libtask_atomic_add(&ncpu, 1);

    libtask_task_pool_switch(io_pool, NULL);
    usleep(10);

    libtask_atomic_add(&nio, 1);
    libtask_task_pool_switch(cpu_pool, NULL);
  }
  libtask_atomic_sub(&ntasks, 1);
  return 0;
}

void *
tmain(void *arg_)
{
  do {
    libtask_task_t *task = NULL;
    if (libtask_task_pool_pop_front(io_pool, &task) == 0) {
      CHECK(libtask_task_execute(task) == 0);
    }

    if (libtask_task_pool_pop_front(cpu_pool, &task) == 0) {
      CHECK(libtask_task_execute(task) == 0);
    }
  } while (libtask_atomic_load(&ntasks) > 0);

  CHECK(libtask_get_task_pool_size(io_pool) == 0);
  CHECK(libtask_get_task_pool_size(cpu_pool) == 0);
  return NULL;
}

int
main(int argc, char *argv[])
{
  CHECK(libtask_task_pool_create(&io_pool) == 0);
  CHECK(libtask_task_pool_create(&cpu_pool) == 0);
  CHECK(io_pool);
  CHECK(cpu_pool);

  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, work, NULL, TASK_STACK_SIZE) == 0);
  CHECK(libtask_task_pool_insert(cpu_pool, &task) == 0);

  pthread_t thread;
  CHECK(pthread_create(&thread, NULL, tmain, NULL) == 0);
  CHECK(pthread_join(thread, NULL) == 0);

  CHECK(libtask_task_pool_unref(io_pool) == 0);
  CHECK(libtask_task_pool_unref(cpu_pool) == 0);
  CHECK(libtask_task_finalize(&task) == 0);

  CHECK(nio == ncpu);
  return 0;
}
