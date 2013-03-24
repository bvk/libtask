//
// Libtask: A thread-safe coroutine library.
//
// Copyright (C) 2013  BVK Chaitanya
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>

#include "libtask/libtask.h"

#define CHECK(x) do { if (!(x)) { perror(""); assert(0); } } while (0)
#define DEBUG(fmt,...) /* printf */ (fmt, ##__VA_ARGS__)

#define NTHREADS 20
#define NITEMS 1000

#define NPRODUCERS 20
#define NCONSUMERS 30

#define MAXITEMS 5
#define TASK_STACK_SIZE (64*1024)

int32_t buffer[MAXITEMS];
libtask_spinlock_t spinlock;
libtask_semaphore_t nfree;
libtask_semaphore_t navail;

int32_t consumed[NITEMS];
int32_t produced[NITEMS];

int32_t nproduced = 0;
int32_t nconsumed = 0;

int32_t producer_next = 0;
int32_t consumer_next = 0;

int
producer(void *arg_)
{
  while (true) {
    // All tasks must exist after NITEMS.
    if (libtask_atomic_add(&nproduced, 1) >= NITEMS) {
      break;
    }

    int32_t value = random();
    libtask_semaphore_down(&nfree);
    libtask_spinlock_lock(&spinlock);

    int32_t index = producer_next++;
    produced[index] = value;
    buffer[index % MAXITEMS] = value;

    libtask_spinlock_unlock(&spinlock);
    libtask_semaphore_up(&navail);
  }
  static int32_t nfinished;
  DEBUG("producer %d finished\n", libtask_atomic_add(&nfinished, 1));
  return 0;
}

int
consumer(void *arg_)
{
  while (true) {
    // All tasks must exist after NITEMS.
    if (libtask_atomic_add(&nconsumed, 1) >= NITEMS) {
      break;
    }

    libtask_semaphore_down(&navail);
    libtask_spinlock_lock(&spinlock);

    int32_t index = consumer_next++;
    int32_t value = buffer[index % MAXITEMS];
    consumed[index] = value;

    libtask_spinlock_unlock(&spinlock);
    libtask_semaphore_up(&nfree);
  }
  static int32_t nfinished;
  DEBUG("consumer %d finished\n", libtask_atomic_add(&nfinished, 1));
  return 0;
}

int
main(int argc, char *argv[])
{
  libtask_spinlock_initialize(&spinlock);
  libtask_semaphore_initialize(&nfree, MAXITEMS);
  libtask_semaphore_initialize(&navail, 0);

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

  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_stop(&pool, threads[i]) == 0);
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  for (int i = 0; i < NCONSUMERS; i++) {
    CHECK(libtask_task_unref(&consumers[i]) == 0);
  }
  for (int i = 0; i < NPRODUCERS; i++) {
    CHECK(libtask_task_unref(&producers[i]) == 0);
  }

  CHECK(libtask_task_pool_unref(&pool) == 0);
  libtask_semaphore_finalize(&nfree);
  libtask_semaphore_finalize(&navail);
  libtask_spinlock_finalize(&spinlock);

  // Verify that producers and consumers have correct values in the
  // correct order.

  for (int i = 0; i < NITEMS; i++) {
    CHECK(produced[i] == consumed[i]);
  }

  return 0;
}
