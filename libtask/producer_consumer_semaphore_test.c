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

#include <argp.h>

#include "libtask/libtask.h"
#include "libtask/util/log.h"
#include "libtask/util/test.h"

#define TASK_STACK_SIZE (64*1024)

static int32_t num_threads = 10;
static int32_t num_items = 20000;
static int32_t num_producers = 20;
static int32_t num_consumers = 30;
static int32_t max_buffer_size = 5;

static struct argp_option options[] = {
  {"num-threads",   0, "N", 0, "Number of threads to use with the task-pool."},
  {"num-items",     1, "N", 0, "Number of items to produce and consume."},
  {"num-producers", 2, "N", 0, "Number of producers."},
  {"num-consumers", 3, "N", 0, "Number of consumers."},
  {"max-buffer-size", 4, "N", 0, "Maximum no. of items queued in the buffer."},
  {0}
};

int32_t *buffer;
libtask_spinlock_t spinlock;
libtask_semaphore_t nfree;
libtask_semaphore_t navail;

int32_t *consumed;
int32_t *produced;

int32_t nproduced = 0;
int32_t nconsumed = 0;

int32_t producer_next = 0;
int32_t consumer_next = 0;

int
producer(void *arg_)
{
  while (true) {
    // All tasks must exist after num_items.
    if (libtask_atomic_add(&nproduced, 1) >= num_items) {
      break;
    }

    int32_t value = random();
    libtask_semaphore_down(&nfree);
    libtask_spinlock_lock(&spinlock);

    int32_t index = producer_next++;
    produced[index] = value;
    buffer[index % max_buffer_size] = value;

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
    // All tasks must exist after num_items.
    if (libtask_atomic_add(&nconsumed, 1) >= num_items) {
      break;
    }

    libtask_semaphore_down(&navail);
    libtask_spinlock_lock(&spinlock);

    int32_t index = consumer_next++;
    int32_t value = buffer[index % max_buffer_size];
    consumed[index] = value;

    libtask_spinlock_unlock(&spinlock);
    libtask_semaphore_up(&nfree);
  }
  static int32_t nfinished;
  DEBUG("consumer %d finished\n", libtask_atomic_add(&nfinished, 1));
  return 0;
}

static error_t
parse_options(int key, char *arg, struct argp_state *state)
{
  switch(key) {
  case 0: // num-threads
    if (!strtoint32(arg, 10, &num_threads) || num_threads <= 0) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 1: // num-items
    if (!strtoint32(arg, 10, &num_items) || num_items <= 0) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 2: // num-producers
    if (!strtoint32(arg, 10, &num_producers) || num_producers <= 0) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 3: // num-consumers
    if (!strtoint32(arg, 10, &num_consumers) || num_consumers <= 0) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  case 4: // max-buffer-size
    if (!strtoint32(arg, 10, &max_buffer_size) || max_buffer_size <= 0) {
      argp_error(state, "Invalid value %s for --%s\n", arg, options[key].name);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  struct argp argp = { options, parse_options };
  argp_parse(&argp, argc, argv, 0, 0, 0);

  CHECK(buffer = malloc(sizeof (int32_t) * max_buffer_size));
  CHECK(consumed = malloc(sizeof (int32_t) * num_items));
  CHECK(produced = malloc(sizeof (int32_t) * num_items));

  libtask_spinlock_initialize(&spinlock);
  libtask_semaphore_initialize(&nfree, max_buffer_size);
  libtask_semaphore_initialize(&navail, 0);

  libtask_task_pool_t pool;
  CHECK(libtask_task_pool_initialize(&pool) == 0);

  libtask_task_t consumers[num_consumers];
  for (int i = 0; i < num_consumers; i++) {
    CHECK(libtask_task_initialize(&consumers[i], &pool, consumer, NULL,
				  TASK_STACK_SIZE) == 0);
  }

  libtask_task_t producers[num_producers];
  for (int i = 0; i < num_producers; i++) {
    CHECK(libtask_task_initialize(&producers[i], &pool, producer, NULL,
				  TASK_STACK_SIZE) == 0);
  }

  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    CHECK(libtask_task_pool_start(&pool, &threads[i]) == 0);
  }

  // Wait for tasks to finish.
  for (int i = 0; i < num_consumers; i++) {
    CHECK(libtask_task_wait(&consumers[i]) == 0);
  }
  for (int i = 0; i < num_producers; i++) {
    CHECK(libtask_task_wait(&producers[i]) == 0);
  }

  for (int i = 0; i < num_threads; i++) {
    CHECK(libtask_task_pool_stop(&pool, threads[i]) == 0);
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  for (int i = 0; i < num_consumers; i++) {
    CHECK(libtask_task_unref(&consumers[i]) == 0);
  }
  for (int i = 0; i < num_producers; i++) {
    CHECK(libtask_task_unref(&producers[i]) == 0);
  }

  CHECK(libtask_task_pool_unref(&pool) == 0);
  libtask_semaphore_finalize(&nfree);
  libtask_semaphore_finalize(&navail);
  libtask_spinlock_finalize(&spinlock);

  // Verify that producers and consumers have correct values in the
  // correct order.

  for (int i = 0; i < num_items; i++) {
    CHECK(produced[i] == consumed[i]);
  }

  return 0;
}
