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

#define NTHREADS 10
#define NYIELD 10000
#define TASK_STACK_SIZE (16 * 1024)

static int32_t num_threads = 10;
static int32_t num_yield = 10000;

static struct argp_option options[] = {
  {"num-threads",   0, "N", 0, "Number of threads to use with the task-pool."},
  {"num-yield",     1, "N", 0, "Number of yeilds to perform by the task."},
  {0}
};

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


static bool
strtoint32(const char *arg, int base, int32_t *valuep)
{
  char *endptr = NULL;
  long int value = strtol(arg, &endptr, base);
  if (((value == LONG_MIN || value == LONG_MAX) && errno == ERANGE) ||
      (value < INT32_MIN || value > INT32_MAX) ||
      (endptr[0] != '\0')) {
    return false;
  }
  *valuep = (int32_t) value;
  return true;
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

  case 1: // num-yields
    if (!strtoint32(arg, 10, &num_yield) || num_yield <= 0) {
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

  libtask_task_pool_t task_pool;
  CHECK(libtask_task_pool_initialize(&task_pool) == 0);

  int counter = 0;
  libtask_task_t task;
  CHECK(libtask_task_initialize(&task, &task_pool, work, &counter,
				TASK_STACK_SIZE) == 0);

  pthread_t pthread[NTHREADS];
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_start(&task_pool, &pthread[i]) == 0);
  }
  CHECK(libtask_task_wait(&task) == 0);
  for (int i = 0; i < NTHREADS; i++) {
    CHECK(libtask_task_pool_stop(&task_pool, pthread[i]) == 0);
    CHECK(pthread_join(pthread[i], NULL) == 0);
  }

  CHECK(counter == NYIELD);

  CHECK(libtask_task_unref(&task) == 0);
  CHECK(libtask_task_pool_unref(&task_pool) == 0);
  return 0;
}
