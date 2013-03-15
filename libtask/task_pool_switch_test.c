#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { assert(0); } } while (0)

#define NITERATIONS 1000

#define TASK_STACK_SIZE (16 * 1024)

static libtask_task_pool_t *io_pool;
static libtask_task_pool_t *cpu_pool;

static int32_t nio = 0;
static int32_t ncpu = 0;
static int32_t nswitch = 0;

int
work(void *arg_)
{
  for (int i = 0; i < NITERATIONS; i++) {
    CHECK(libtask_task_pool_schedule(io_pool) == 0);

    libtask_atomic_add(&nio, 1);
    usleep(10);

    CHECK(libtask_task_pool_schedule(cpu_pool) == 0);
    libtask_atomic_add(&ncpu, 1);

    libtask_atomic_add(&nswitch, 1);
  }
  return 0;
}

void *
tmain(void *arg_)
{
  libtask_task_pool_t *pool = (libtask_task_pool_t *)arg_;
  do {
    CHECK(libtask_task_pool_execute(pool) == 0);
  } while (libtask_atomic_load(&nswitch) < NITERATIONS);

  return 0;
}

int
main(int argc, char *argv[])
{
  CHECK(libtask_task_pool_create(&io_pool) == 0);
  CHECK(libtask_task_pool_create(&cpu_pool) == 0);
  CHECK(io_pool);
  CHECK(cpu_pool);

  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, cpu_pool, work, NULL,
				TASK_STACK_SIZE) == 0);

  pthread_t io_thread;
  CHECK(pthread_create(&io_thread, NULL, tmain, io_pool) == 0);
  pthread_t cpu_thread;
  CHECK(pthread_create(&cpu_thread, NULL, tmain, cpu_pool) == 0);
  CHECK(pthread_join(io_thread, NULL) == 0);
  CHECK(pthread_join(cpu_thread, NULL) == 0);

  CHECK(libtask_task_pool_unref(cpu_pool) == 0);
  CHECK(libtask_task_pool_unref(io_pool) == 0);
  CHECK(libtask_task_unref(&task) == 0);
  return 0;
}
