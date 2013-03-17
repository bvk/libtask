//
// Test case that implements bounded buffer multi-producer and
// multi-consumer problem using tasks.
//
// 1. Testcase uses multiple consumer and producer tasks to create
//    NITEMS into a bounded buffer of size MAXITEMS.
//
// 2. These producer and consumer tasks are all under single task-pool
//    that uses NTHREADS.
//
// 3. At the end we verify that consumer tasks have received values
//    from the producer tasks in the same order.
//

#include <stdio.h>
#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)

#define NTHREADS 100
#define NITEMS 20000

#define NPRODUCERS 20
#define NCONSUMERS 30

#define MAXITEMS 5

#define TASK_STACK_SIZE (64*1024)

int32_t size;
int32_t buffer[MAXITEMS];
pthread_spinlock_t spinlock;
libtask_wait_queue_t full;
libtask_wait_queue_t empty;

int32_t consumed[NITEMS];
int32_t produced[NITEMS];

int
producer(void *arg_)
{
  static int32_t next = 0;

  while (true) {
    int32_t index = -1;
    int32_t value = random();

    pthread_spin_lock(&spinlock);
    while (true) {
      if (next >= NITEMS) {
	pthread_spin_unlock(&spinlock);
	return 0;
      }

      if (size == 0) {
	libtask_wait_queue_wake_up(&empty);
      }

      if (size < MAXITEMS) {
	index = next++;
	produced[index] = value;

	buffer[index % MAXITEMS] = value;
	size++;
	break;
      }

      libtask_wait_queue_sleep(&full, &spinlock);
    }
    pthread_spin_unlock(&spinlock);
  }
}

int
consumer(void *arg_)
{
  static int32_t next = 0;

  while (true) {
    int32_t index = -1;
    int32_t value = -1;

    pthread_spin_lock(&spinlock);
    while (true) {
      if (next >= NITEMS) {
	pthread_spin_unlock(&spinlock);
	return 0;
      }

      if (size == MAXITEMS) {
	libtask_wait_queue_wake_up(&full);
      }

      if (size > 0) {
	index = next++;
	value = buffer[index % MAXITEMS];

	consumed[index] = value;
	size--;
	break;
      }

      libtask_wait_queue_sleep(&empty, &spinlock);
    }
    pthread_spin_unlock(&spinlock);
  }
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
  CHECK(libtask_wait_queue_initialize(&full) == 0);
  CHECK(libtask_wait_queue_initialize(&empty) == 0);

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
  CHECK(libtask_wait_queue_finalize(&full) == 0);
  CHECK(libtask_wait_queue_finalize(&empty) == 0);

  // Verify that producers and consumers have correct values in the
  // correct order.

  for (int i = 0; i < NITEMS; i++) {
    CHECK(produced[i] == consumed[i]);
  }

  return 0;
}
