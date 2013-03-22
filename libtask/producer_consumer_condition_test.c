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
#include <string.h>
#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)
#define DEBUG(fmt,...) /* printf */ (fmt, ##__VA_ARGS__)

#define NTHREADS 10
#define NITEMS 20000

#define NPRODUCERS 20
#define NCONSUMERS 30

#define MAXITEMS 5

#define TASK_STACK_SIZE (64*1024)

int32_t size;
int32_t buffer[MAXITEMS];
libtask_spinlock_t spinlock;
libtask_condition_t full;
libtask_condition_t empty;

int32_t consumed[NITEMS];
int32_t produced[NITEMS];

int32_t producer_next;
int32_t consumer_next;

int
producer(void *arg_)
{
  while (true) {
    int32_t index = -1;
    int32_t value = random();

    libtask_spinlock_lock(&spinlock);
    while (true) {
      if (producer_next >= NITEMS) {
	libtask_spinlock_unlock(&spinlock);
	static int32_t nfinished;
	DEBUG("producer %d finished\n", libtask_atomic_add(&nfinished, 1));
	return 0;
      }

      if (size == 0) {
	// Multiple consumers may be waiting for the last element, so
	// we need to broadcast so that all of them will wake up and
	// realize that they reached the end.
	libtask_condition_broadcast(&empty);
      }

      if (size < MAXITEMS) {
	index = producer_next++;
	produced[index] = value;

	buffer[index % MAXITEMS] = value;
	size++;
	break;
      }

      libtask_condition_wait(&full);
    }
    libtask_spinlock_unlock(&spinlock);
  }
}

int
consumer(void *arg_)
{
  while (true) {
    int32_t index = -1;
    int32_t value = -1;

    libtask_spinlock_lock(&spinlock);
    while (true) {
      if (consumer_next >= NITEMS) {
	libtask_spinlock_unlock(&spinlock);
	static int32_t nfinished;
	DEBUG("consumer %d finished\n", libtask_atomic_add(&nfinished, 1));
	return 0;
      }

      if (size == MAXITEMS) {
	libtask_condition_broadcast(&full);
      }

      if (size > 0) {
	index = consumer_next++;
	value = buffer[index % MAXITEMS];

	consumed[index] = value;
	size--;
	break;
      }

      libtask_condition_wait(&empty);
    }
    libtask_spinlock_unlock(&spinlock);
  }
}

int
main(int argc, char *argv[])
{
  libtask_spinlock_initialize(&spinlock);
  libtask_condition_initialize(&full, &spinlock);
  libtask_condition_initialize(&empty, &spinlock);

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
    CHECK(libtask_task_pool_start(&pool, &threads[i]) == 0);
  }

  // Wait for tasks to finish.
  for (int i = 0; i < NCONSUMERS; i++) {
    CHECK(libtask_task_wait(&consumers[i]) == 0);
  }
  for (int i = 0; i < NPRODUCERS; i++) {
    CHECK(libtask_task_wait(&producers[i]) == 0);
  }

  // Stop the task-pool threads.
  for (int i = 0; i < NTHREADS; i++) {
    libtask_task_pool_stop(&pool, threads[i]);
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  for (int i = 0; i < NCONSUMERS; i++) {
    CHECK(libtask_task_unref(&consumers[i]) == 0);
  }
  for (int i = 0; i < NPRODUCERS; i++) {
    CHECK(libtask_task_unref(&producers[i]) == 0);
  }

  CHECK(libtask_task_pool_unref(&pool) == 0);
  libtask_condition_finalize(&full);
  libtask_condition_finalize(&empty);
  libtask_spinlock_finalize(&spinlock);

  // Verify that producers and consumers have correct values in the
  // correct order.

  for (int i = 0; i < NITEMS; i++) {
    CHECK(produced[i] == consumed[i]);
  }

  return 0;
}
