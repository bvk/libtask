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

//
// Test case that implements bounded buffer multi-producer and
// multi-consumer problem using tasks.
//
// 1. Testcase uses multiple consumer and producer tasks to create
//    num_items into a bounded buffer of size max_buffer_size.
//
// 2. These producer and consumer tasks are all under single task-pool
//    that uses num_threads.
//
// 3. At the end we verify that consumer tasks have received values
//    from the producer tasks in the same order.
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

int32_t size;
int32_t *buffer;
libtask_spinlock_t spinlock;
libtask_condition_t full;
libtask_condition_t empty;

int32_t *consumed;
int32_t *produced;

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
      if (producer_next >= num_items) {
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

      if (size < max_buffer_size) {
	index = producer_next++;
	produced[index] = value;

	buffer[index % max_buffer_size] = value;
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
      if (consumer_next >= num_items) {
	libtask_spinlock_unlock(&spinlock);
	static int32_t nfinished;
	DEBUG("consumer %d finished\n", libtask_atomic_add(&nfinished, 1));
	return 0;
      }

      if (size == max_buffer_size) {
	libtask_condition_broadcast(&full);
      }

      if (size > 0) {
	index = consumer_next++;
	value = buffer[index % max_buffer_size];

	consumed[index] = value;
	size--;
	break;
      }

      libtask_condition_wait(&empty);
    }
    libtask_spinlock_unlock(&spinlock);
  }
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
  libtask_condition_initialize(&full, &spinlock);
  libtask_condition_initialize(&empty, &spinlock);

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

  // Stop the task-pool threads.
  for (int i = 0; i < num_threads; i++) {
    libtask_task_pool_stop(&pool, threads[i]);
    CHECK(pthread_join(threads[i], NULL) == 0);
  }

  for (int i = 0; i < num_consumers; i++) {
    CHECK(libtask_task_unref(&consumers[i]) == 0);
  }
  for (int i = 0; i < num_producers; i++) {
    CHECK(libtask_task_unref(&producers[i]) == 0);
  }

  CHECK(libtask_task_pool_unref(&pool) == 0);
  libtask_condition_finalize(&full);
  libtask_condition_finalize(&empty);
  libtask_spinlock_finalize(&spinlock);

  // Verify that producers and consumers have correct values in the
  // correct order.

  for (int i = 0; i < num_items; i++) {
    CHECK(produced[i] == consumed[i]);
  }

  return 0;
}
