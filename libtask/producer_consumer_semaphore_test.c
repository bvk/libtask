#include <stdio.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)

#define NTHREADS 100
#define NITEMS 20000

#define NPRODUCERS 20
#define NCONSUMERS 30

#define MAXITEMS 5
#define TASK_STACK_SIZE (64*1024)

int32_t buffer[MAXITEMS];
pthread_spinlock_t spinlock;
libtask_semaphore_t nfree;
libtask_semaphore_t navail;

int32_t consumed[NITEMS];
int32_t produced[NITEMS];

int
producer(void *arg_)
{
  static int32_t next = 0;
  static int32_t nproduced = 0;

  while (true) {
    // All tasks must exist after NITEMS.
    if (libtask_atomic_add(&nproduced, 1) >= NITEMS) {
      break;
    }

    int32_t value = random();
    libtask_semaphore_down(&nfree);
    pthread_spin_lock(&spinlock);

    int32_t index = next++;
    produced[index] = value;
    buffer[index % MAXITEMS] = value;

    pthread_spin_unlock(&spinlock);
    libtask_semaphore_up(&navail);
  }
  return 0;
}

int
consumer(void *arg_)
{
  static int32_t next = 0;
  static int32_t nconsumed = 0;

  while (true) {
    // All tasks must exist after NITEMS.
    if (libtask_atomic_add(&nconsumed, 1) >= NITEMS) {
      break;
    }

    libtask_semaphore_down(&navail);
    pthread_spin_lock(&spinlock);

    int32_t index = next++;
    int32_t value = buffer[index % MAXITEMS];
    consumed[index] = value;

    pthread_spin_unlock(&spinlock);
    libtask_semaphore_up(&nfree);
  }
  return 0;
}

void *
tmain(void *arg_)
{
  libtask_task_pool_t *pool = (libtask_task_pool_t *) arg_;
  libtask_task_pool_execute(pool);
  return NULL;
}

int
main(int argc, char *argv[])
{
  CHECK(pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE) == 0);
  CHECK(libtask_semaphore_initialize(&nfree, MAXITEMS) == 0);
  CHECK(libtask_semaphore_initialize(&navail, 0) == 0);

  libtask_task_pool_t pool;
  CHECK(libtask_task_pool_initialize(&pool) == 0);

  libtask_task_t consumers[NCONSUMERS];
  for (int i = 0; i < NCONSUMERS; i++) {
    CHECK(libtask_task_initialize(&consumers[i], &pool, consumer, NULL,
				  TASK_STACK_SIZE) == 0);
  }

  libtask_task_t producers[NPRODUCERS];
  for (int i = 0; i < NPRODUCERS; i++) {
    CHECK(libtask_task_initialize(&producers[i], &pool, producer, NULL,
				  TASK_STACK_SIZE) == 0);
  }

  pthread_t threads[NTHREADS];
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(pthread_create(&threads[i], NULL, tmain, &pool) == 0);
  }
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  for (int i = 0; i < NCONSUMERS; i++) {
    CHECK(libtask_task_unref(&consumers[i]) == 0);
  }
  for (int i = 0; i < NPRODUCERS; i++) {
    CHECK(libtask_task_unref(&producers[i]) == 0);
  }

  CHECK(libtask_task_pool_unref(&pool) == 0);
  CHECK(libtask_semaphore_finalize(&nfree) == 0);
  CHECK(libtask_semaphore_finalize(&navail) == 0);

  // Verify that producers and consumers have correct values in the
  // correct order.

  for (int i = 0; i < NITEMS; i++) {
    CHECK(produced[i] == consumed[i]);
  }

  return 0;
}
